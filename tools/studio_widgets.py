"""Desktop-only workspace navigation, safe documents and asynchronous builds."""

import codecs
from pathlib import Path
import shlex
import shutil
import sys

from PyQt6.QtCore import QIODevice, QProcess, QProcessEnvironment, QSaveFile, QSettings, Qt, QTimer, QUrl
from PyQt6.QtGui import QDesktopServices
from PyQt6.QtWidgets import (
    QApplication, QCheckBox, QComboBox, QFileDialog, QHBoxLayout, QLabel,
    QLineEdit, QListWidget, QMessageBox, QPlainTextEdit, QProgressBar,
    QPushButton, QSplitter, QStyle, QToolBar, QVBoxLayout, QWidget,
)

from studio_project import FileSnapshot, game_names


def save_document(parent, path, data, snapshot=None):
    """Commit one document atomically; external modifications must be reconciled."""
    path = Path(path)
    try:
        if snapshot is not None:
            snapshot.check()
        elif path.exists() and QMessageBox.question(
            parent, "Replace file?", str(path),
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        ) != QMessageBox.StandardButton.Yes:
            return False
        if isinstance(data, str):
            data = data.encode("utf-8")
        dest = QSaveFile(str(path))
        if not dest.open(QIODevice.OpenModeFlag.WriteOnly):
            raise OSError(dest.errorString())
        if dest.write(data) != len(data) or not dest.commit():
            raise OSError(dest.errorString())
        if snapshot is not None:
            snapshot.refresh()
        return True
    except (OSError, ValueError) as exc:
        QMessageBox.warning(parent, "File not saved", str(exc))
        return False


def discard_edits(parent):
    return QMessageBox.question(
        parent, "Unsaved workspace", "Discard unsaved workspace edits?",
        QMessageBox.StandardButton.Discard | QMessageBox.StandardButton.Cancel,
        QMessageBox.StandardButton.Cancel,
    ) == QMessageBox.StandardButton.Discard


def mount_workspace(window, tabs, project, kind, switch_game, reload_project):
    """Keep all editors reachable without a horizontally clipped tab bar."""
    tabs.tabBar().hide()
    nav = QListWidget()
    nav.setMinimumWidth(165)
    nav.setMaximumWidth(235)
    for i in range(tabs.count()):
        nav.addItem(tabs.tabText(i))
    nav.currentRowChanged.connect(tabs.setCurrentIndex)
    tabs.currentChanged.connect(nav.setCurrentRow)
    search = QLineEdit()
    search.setPlaceholderText("Find workspace")
    search.setClearButtonEnabled(True)
    search.textChanged.connect(lambda query: [
        nav.item(i).setHidden(query.casefold() not in nav.item(i).text().casefold())
        for i in range(nav.count())
    ])
    left = QWidget()
    layout = QVBoxLayout(left)
    layout.setContentsMargins(0, 0, 0, 0)
    layout.addWidget(search)
    layout.addWidget(nav)
    split = QSplitter()
    split.addWidget(left)
    split.addWidget(tabs)
    split.setStretchFactor(1, 1)
    split.setSizes([185, 1040])
    window.setCentralWidget(split)
    window.workspace_tabs = tabs
    window.workspace_nav = nav

    if hasattr(window, "project_toolbar"):
        window.removeToolBar(window.project_toolbar)
        window.project_toolbar.deleteLater()
    toolbar = QToolBar("Project", window)
    toolbar.setObjectName("projectToolbar")
    toolbar.setMovable(False)
    toolbar.addWidget(QLabel(" Game "))
    combo = QComboBox()
    combo.addItems(game_names(project.root))
    combo.setCurrentText(project.game)
    combo.setMinimumWidth(150)
    combo.currentTextChanged.connect(switch_game)
    toolbar.addWidget(combo)
    action = toolbar.addAction(window.style().standardIcon(QStyle.StandardPixmap.SP_BrowserReload), "Reload project")
    action.setToolTip("Reload the selected game's files from disk")
    action.triggered.connect(reload_project)
    action = toolbar.addAction(window.style().standardIcon(QStyle.StandardPixmap.SP_DirOpenIcon), "Open source folder")
    source = project.art_source if kind == "art" else project.sound
    action.triggered.connect(lambda: QDesktopServices.openUrl(QUrl.fromLocalFile(str(source))))
    action = toolbar.addAction(window.style().standardIcon(QStyle.StandardPixmap.SP_DialogHelpButton), "Studio manual")
    action.triggered.connect(lambda: QDesktopServices.openUrl(QUrl.fromLocalFile(str(project.root / "docs" / "DESKTOP_STUDIOS.md"))))
    window.addToolBar(toolbar)
    window.project_toolbar = toolbar
    window.project_combo = combo
    window.statusBar().showMessage(f"{project.game}  |  ROM {project.game_id}  |  Sources: {source}")
    settings = QSettings("Eagle Software", f"{kind}-studio")
    index = tabs.indexOf(next((tabs.widget(i) for i in range(tabs.count())
                             if tabs.tabText(i) == settings.value("workspace", "")), tabs.widget(0)))
    nav.setCurrentRow(max(0, index))
def update_workspace_project(window, project, kind):
    """Update window toolbar, combo, and status bar for a newly loaded project."""
    source = project.art_source if kind == "art" else project.sound
    if hasattr(window, "project_combo"):
        window.project_combo.blockSignals(True)
        window.project_combo.setCurrentText(project.game)
        window.project_combo.blockSignals(False)
    if window.statusBar():
        window.statusBar().showMessage(f"{project.game}  |  ROM {project.game_id}  |  Sources: {source}")


class BuildPanel(QWidget):
    """Delegate dependencies to make, never reconstruct a pipeline in the GUI."""

    def __init__(self, project, kind):
        super().__init__()
        self.project = project
        self.kind = kind
        self._proc = None
        self._queue = []
        self.cancelled = False
        layout = QVBoxLayout(self)
        row = QHBoxLayout()
        self.targets = QComboBox()
        self.targets.addItems(["art", "sfix"] if kind == "art" else [
            "sound", "samples", "vrom", "fmpatches", "fm", "mml", "ssgconfig", "ssg", "m1rom"])
        self.targets.currentTextChanged.connect(self._show_command)
        row.addWidget(self.targets)
        self.run_button = QPushButton("Build selected")
        self.run_button.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_MediaPlay))
        self.run_button.clicked.connect(lambda: self.start_targets([self.targets.currentText()]))
        row.addWidget(self.run_button)
        self.full_button = QPushButton("Build graphics" if kind == "art" else "Build sound")
        self.full_button.clicked.connect(lambda: self.start_targets(["art", "sfix"] if kind == "art" else ["sound"]))
        row.addWidget(self.full_button)
        self.stop_button = QPushButton("Stop")
        self.stop_button.setIcon(self.style().standardIcon(QStyle.StandardPixmap.SP_MediaStop))
        self.stop_button.clicked.connect(self.stop)
        self.stop_button.setEnabled(False)
        row.addWidget(self.stop_button)
        row.addStretch()
        layout.addLayout(row)
        self.enable_build = QCheckBox("Enable builds in this checkout")
        self.enable_build.setToolTip("Builds update generated game files, shared driver tables and ROMs. Coordinate with other sessions first.")
        layout.addWidget(self.enable_build)
        self.command = QLineEdit()
        self.command.setReadOnly(True)
        layout.addWidget(self.command)
        self.progress = QProgressBar()
        self.progress.setRange(0, 1)
        self.progress.setValue(0)
        layout.addWidget(self.progress)
        self.state = QLabel("Ready")
        layout.addWidget(self.state)
        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setMaximumBlockCount(15000)
        layout.addWidget(self.log, 1)
        export = QPushButton("Export log")
        export.clicked.connect(self._export_log)
        layout.addWidget(export)
        self._show_command()

    def set_project(self, project):
        """Update active project for make delegation."""
        self.project = project
        self._show_command()

    @property
    def busy(self):
        return self._proc is not None

    def _show_command(self, *_):
        self.command.setText(shlex.join(self.project.make_command(self.targets.currentText())))

    def start_targets(self, targets):
        if self.busy:
            return
        if not self.enable_build.isChecked():
            QMessageBox.information(self, "Builds disabled", "Enable builds only when no other session is building this checkout.")
            return
        if QMessageBox.question(self, "Build generated files?",
                                f"Game: {self.project.game}\nTargets: {', '.join(targets)}\n"
                                "Generated assets, shared tables and ROMs may be replaced.",
                                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
                                QMessageBox.StandardButton.No) != QMessageBox.StandardButton.Yes:
            return
        self._queue = list(targets)
        self.cancelled = False
        self._next()

    def _next(self):
        if not self._queue:
            self._idle("Build complete")
            return
        target = self._queue.pop(0)
        cmd = self.project.make_command(target)
        executable = shutil.which(cmd[0])
        if not executable:
            self._queue.clear()
            self._idle("make was not found on PATH")
            return
        self.log.appendPlainText(f"\n[{self.project.game}] {shlex.join(cmd)}")
        self.state.setText(f"Building {target}")
        self.progress.setRange(0, 0)
        for widget in (self.targets, self.run_button, self.full_button, self.enable_build):
            widget.setEnabled(False)
        self.stop_button.setEnabled(True)
        self._decoder = codecs.getincrementaldecoder("utf-8")("replace")
        proc = QProcess(self)
        proc.setWorkingDirectory(str(self.project.root))
        proc.setProcessChannelMode(QProcess.ProcessChannelMode.MergedChannels)
        env = QProcessEnvironment.systemEnvironment()
        env.insert("PYTHONDONTWRITEBYTECODE", "1")
        env.insert("PYTHONUNBUFFERED", "1")
        proc.setProcessEnvironment(env)
        proc.readyReadStandardOutput.connect(self._read)
        proc.errorOccurred.connect(self._error)
        proc.finished.connect(self._finished)
        self._proc = proc
        proc.start(executable, cmd[1:])

    def _read(self):
        if self._proc:
            text = self._decoder.decode(bytes(self._proc.readAllStandardOutput()))
            cursor = self.log.textCursor()
            cursor.movePosition(cursor.MoveOperation.End)
            cursor.insertText(text)
            self.log.setTextCursor(cursor)
            self.log.ensureCursorVisible()

    def _error(self, error):
        if error == QProcess.ProcessError.FailedToStart and self._proc:
            message = self._proc.errorString()
            self._proc.deleteLater()
            self._proc = None
            self._queue.clear()
            self._idle(f"Failed to start: {message}")

    def _finished(self, code, status):
        if self._proc is None:
            return
        self._read()
        self._proc.deleteLater()
        self._proc = None
        if self.cancelled or code or status != QProcess.ExitStatus.NormalExit:
            self._queue.clear()
            self._idle("Cancelled; partial outputs may exist" if self.cancelled else f"Failed: exit {code}")
        else:
            self._next()

    def _idle(self, message):
        self.state.setText(message)
        self.log.appendPlainText(message)
        self.progress.setRange(0, 1)
        self.progress.setValue(1 if message == "Build complete" else 0)
        for widget in (self.targets, self.run_button, self.full_button, self.enable_build):
            widget.setEnabled(True)
        self.stop_button.setEnabled(False)

    def stop(self):
        self._queue.clear()
        self.cancelled = True
        if self._proc:
            proc = self._proc
            proc.terminate()
            QTimer.singleShot(2000, lambda: proc.kill() if self._proc is proc else None)

    def _export_log(self):
        filename, _ = QFileDialog.getSaveFileName(self, "Export build log", "studio-build.txt", "Text (*.txt)")
        if filename:
            save_document(self, filename, self.log.toPlainText())

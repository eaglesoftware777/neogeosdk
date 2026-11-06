"""Checks for the ordinary-input movement trace; no game-memory mutation."""


def validate(rows, tiles, palette):
    if not rows or rows[-1]['phase'] != 'complete':
        raise ValueError('Movement replay incomplete; inspect section.csv')
    rows = [{k: (v if k == 'phase' else float(v) if k == 'seconds' else int(v))
             for k, v in r.items()} for r in rows]
    if len({r['hero'] for r in rows}) != 1 or any(r['stage'] != 0 for r in rows):
        raise ValueError('Hero/stage changed during movement replay')
    if len({r['lives'] for r in rows}) != 1 or any(r['hp'] < rows[0]['hp'] for r in rows):
        raise ValueError('Movement replay lost health/life')
    normal = [r for r in rows if not any(r[k] for k in ('veil', 'hurt', 'flash'))]
    if len(normal) < 200 or any(r[f'pal{i}'] != palette[i] for r in normal for i in range(1, 16)):
        raise ValueError('Ordinary heroine palette changed')
    for i, r in enumerate(rows):
        if r['scb_rows'] != 4:
            raise ValueError(f'Missing heroine sprite rows at sample {i}')
        candidates = rows[max(0, i - 1):i + 1]
        if not any(all(r[f'r{y}c{x}'] == p['tile'] + y * 5 + (4 - x if p['facing'] else x)
                       for y in range(4) for x in range(5)) for p in candidates):
            raise ValueError(f'Mixed/incomplete body group at sample {i}')
        if r['tile'] in tiles[60:76] and (r['sx'] != 255 or r['sy'] != 255):
            raise ValueError('Hardware shrink applied to gait art')
    coverage = {}
    for direction, facing in (('right', 0), ('left', 1)):
        for mode, expected in (('walk', tiles[60:68]), ('run', tiles[68:76])):
            phase = f'{mode}_{direction}'
            observed = {r['r0c0'] - (4 if facing else 0) for r in rows
                        if r['phase'] == phase and r['facing'] == facing}
            if not set(expected) <= observed:
                raise ValueError(f'Incomplete {phase} hardware poses: {set(expected) - observed}')
            coverage[phase] = 8
        stop = [r for r in rows if r['phase'] == f'stop_{direction}']
        if not any(r['vx'] == 0 and r['stop'] for r in stop) or stop[-1]['vx'] or stop[-1]['gait']:
            raise ValueError(f'Missing stop/idle transition after running {direction}')
        firing = [r for r in rows if r['phase'] == f'fire_{direction}']
        if max(r['x'] for r in firing) - min(r['x'] for r in firing) < 40:
            raise ValueError(f'No meaningful stepping while firing {direction}')
        if not any(r['plant'] and r['cast'] for r in firing) or not any(r['gait'] for r in firing):
            raise ValueError(f'Missing planted cast or intervening gait {direction}')
        if any(r['plant'] > 1 and r['vx'] for r in firing):
            raise ValueError('Planted firing slides')
    reverse = [r for r in rows if r['phase'] == 'reverse_left']
    if not any(r['turning'] and r['vx'] > 0 for r in reverse) or not any(r['vx'] < 0 and r['facing'] == 1 for r in reverse):
        raise ValueError('Reversal did not brake and turn')
    jump = [r for r in rows if r['phase'] in ('jump_right', 'land')]
    if not any(r['vy'] < 0 for r in jump) or not any(r['vy'] > 0 for r in jump) or jump[-1]['y'] != 192:
        raise ValueError('Jump did not ascend, descend and land')
    if any(r['cam_y'] != 0 for r in jump):
        raise ValueError('Ordinary road jump moves the vertical camera')
    # Collapse repeated hardware samples before checking per-game-tick contracts.
    updates = [r for i, r in enumerate(rows) if i == 0 or r['tick'] != rows[i - 1]['tick']]
    wall = [r for r in updates if r['phase'] == 'wall_left' and r['x'] == 16]
    if len(wall) < 15 or len({r['distance'] for r in wall[1:]}) != 1:
        raise ValueError('Blocked movement advances gait clock')
    if any(r['gait'] or r['tile'] not in tiles[:3] for r in wall[-15:]):
        raise ValueError('Blocked heroine remains frozen in a gait pose instead of idle')
    clock_samples = 0
    for prev, r in zip(updates, updates[1:]):
        if r['phase'] in ('prepare', 'fire_right', 'fire_left', 'fire_still') or not r['clock']:
            continue
        moved = abs(r['x_fp'] - prev['x_fp'])
        if moved > 1280:
            continue
        units = moved if r['gait'] & 16 else moved + (moved >> 2)
        if r['distance'] != (prev['distance'] + units) % (80 * 256):
            raise ValueError(f'Gait clock differs from actual displacement at tick {r["tick"]}')
        clock_samples += 1
    if clock_samples < 300:
        raise ValueError('Insufficient actual-distance gait coverage')
    return dict(passed=True, hardware_samples=len(rows), game_updates=len(updates),
                complete_body_groups=len(rows), normal_palette_samples=len(normal),
                pose_coverage=coverage, reversal=True, step_then_fire=True,
                jump_landing=True, stable_ground_camera=True, blocked_clock_samples=len(wall),
                actual_distance_clock_samples=clock_samples, full_gait_scale=True)

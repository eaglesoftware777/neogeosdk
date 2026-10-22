"""Validate actual completed frame consumption, separately from live game state."""


def validate(rows):
    if not rows:
        raise ValueError('Publication tap recorded no consumed batch')
    rows = [{k: (float(v) if k == 'seconds' else int(v)) for k, v in r.items()} for r in rows]
    epochs = {r['consumed'] for r in rows}
    if len(epochs) < 100 or max(r['published'] for r in rows) < 100:
        raise ValueError('Insufficient validated producer/consumer handoffs')
    visible = [r for r in rows if r['visible'] and r['owned']]
    bad = [i for i, r in enumerate(rows) if r['visible'] and
           (not r['owned'] or not r['complete'] or r['rows'] != 4)]
    return dict(passed=not bad, consumed_epochs=len(epochs), visible_samples=len(visible),
                mismatching_samples=bad, reference='immutable consumed sprite group, not live producer model')

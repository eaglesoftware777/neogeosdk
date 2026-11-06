"""Normal-control forest completion and gate/door regression contracts."""


def validate(rows, tiles, hero):
    if not rows or rows[-1]['phase'] != 'complete':
        raise ValueError('Forest incomplete; inspect native snapshots and section.csv')
    rows = [{k: (v if k == 'phase' else float(v) if k == 'seconds' else int(v))
             for k, v in row.items()} for row in rows]
    if any(r['hero'] != hero or r['stage'] != 0 for r in rows):
        raise ValueError('Wrong heroine or mission')
    if any(r['lives'] < p['lives'] for p, r in zip(rows, rows[1:])):
        raise ValueError('Forest replay lost a life')
    if rows[-1]['state'] != 2 or rows[-1]['boss_hp'] != 0:
        raise ValueError('Guardian was not defeated')
    if not any(r['key'] for r in rows) or not rows[-1]['gate']:
        raise ValueError('Required key/gate sequence missing')
    door = [r for r in rows if r['phase'] == 'door' and r['state_timer'] > 110]
    if len(door) < 3 or max(r['x'] for r in door) <= min(r['x'] for r in door):
        raise ValueError('No scripted movement into the door')
    door_poses = {r['tile'] for r in door if r['tile'] in tiles[60:68]}
    if not door_poses:
        raise ValueError('Door walk did not use authored walking drawings')
    door_steps = 0
    for previous, current in zip(door, door[1:]):
        # A five-pixel doorway may correctly stay within one held drawing.
        # Check actual 5/4 walking-distance progress instead of padding poses.
        moved = abs(current['x'] - previous['x'])
        if current['distance'] != (previous['distance'] + moved * 320) % (80 * 256):
            raise ValueError('Door gait clock does not match actual walking displacement')
        door_steps += moved
    if not door_steps:
        raise ValueError('No measured scripted walking steps')
    visible = 0
    missing, mixed, late = [], [], []
    pending_request, wait, peak_wait = None, 0, 0
    for i, r in enumerate(rows):
        previous = rows[max(0, i - 1)]
        if not r['visible'] or not previous['visible']:
            continue  # Door intentionally turns the character into light.
        if r['scb_rows'] != 4:
            missing.append(i)
        def matches(tile, facing):
            return all(r[f'r{y}c{x}'] == tile + y * 5 + (4 - x if facing else x)
                       for y in range(4) for x in range(5))
        # A complete older drawing is latency, not fragmented anatomy.
        # Keep the original current/previous-sample latency gate separately.
        if not any(r['r0c0'] - (4 if facing else 0) in tiles and
                   matches(r['r0c0'] - (4 if facing else 0), facing) for facing in (0, 1)):
            mixed.append(i)
        candidates = rows[max(0, i - 1):i + 1]
        if not any(matches(p['tile'], p['facing']) for p in candidates):
            late.append(i)
        request = (r['tile'], r['facing'])
        wait = 0 if matches(*request) else (wait + 1 if request == pending_request else 1)
        peak_wait = max(peak_wait, wait)
        pending_request = request
        visible += 1
    damage = sum(r['hp'] < p['hp'] for p, r in zip(rows, rows[1:]))
    art_spent = sum(max(0, p['art'] - r['art']) for p, r in zip(rows, rows[1:])) if 'art' in rows[0] else None
    return dict(passed=not missing and not mixed and not late, route='forest', normal_inputs_only=True, hardware_samples=len(rows),
                visible_samples=visible, visible_complete_groups=visible - len(set(missing + mixed)),
                seconds=rows[-1]['seconds'] - rows[0]['seconds'],
                missing_body_samples=missing, mixed_body_samples=mixed,
                late_pose_samples=late, peak_request_wait_samples=peak_wait,
                key_gate_guardian=True, door_walk=True, life_lost=False,
                door_drawings=len(door_poses), door_pixels=door_steps,
                lives_gained=rows[-1]['lives'] - rows[0]['lives'],
                hp_start=rows[0]['hp'], hp_end=rows[-1]['hp'], damage_events=damage,
                art_spent=art_spent,
                rescue_mask=rows[-1]['rescues'])

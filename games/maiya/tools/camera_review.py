"""Ordinary-input jumps must hold actual scenery sprite positions steady."""


def validate(rows):
    if not rows or rows[-1]['phase'] != 'complete':
        raise ValueError('Camera route did not finish')
    rows = [{k: (v if k == 'phase' else float(v) if k == 'seconds' else int(v))
             for k, v in r.items()} for r in rows]
    if len({r['hero'] for r in rows}) != 1 or any(r['stage'] for r in rows):
        raise ValueError('Hero or stage changed')
    if len({r['lives'] for r in rows}) != 1 or any(r['hp'] < rows[0]['hp'] for r in rows):
        raise ValueError('Camera route lost health or a life')
    phases = {}
    for terrain, support in (('ground', 192), ('ledge', 88)):
        for cycle in range(1, 4):
            phase = f'{terrain}_jump{cycle}'
            samples = [r for r in rows if r['phase'] == phase]
            if len(samples) < 60 or not any(r['vy'] < 0 for r in samples) or not any(r['vy'] > 0 for r in samples):
                raise ValueError(f'{phase}: missing full jump arc')
            if samples[-1]['y'] != support or samples[-1]['vy']:
                raise ValueError(f'{phase}: did not return to the same footing')
            fields = ['cam_y', 'far_y', 'road_y']
            fields += [f'ledge{i}_{field}' for i in range(9) for field in ('y', 'rows')]
            for field in fields:
                if len({r[field] for r in samples}) != 1:
                    raise ValueError(f'{phase}: rendered scenery moves ({field})')
            if terrain == 'ground' and any(r['cam_y'] for r in samples):
                raise ValueError('Road framing is not zero')
            if terrain == 'ledge' and not any(samples[0][f'ledge{i}_rows'] for i in range(9)):
                raise ValueError('No actual ledge sprites were observed')
            if any(r['canvas_top'] < 32 or r['canvas_bottom'] > 224 or r['scb_rows'] != 4 for r in samples):
                raise ValueError(f'{phase}: heroine clipping or missing rows')
            phases[phase] = dict(samples=len(samples), camera_y=samples[0]['cam_y'],
                                 far_y=samples[0]['far_y'], road_y=samples[0]['road_y'])
    return dict(passed=True, hardware_samples=len(rows), stationary_scenery_jumps=6, phases=phases)


def validate_bricks(rows):
    if not rows or rows[-1]['phase'] != 'complete':
        raise ValueError('Road-bricks-road replay did not finish')
    rows = [{k: (v if k == 'phase' else float(v) if k == 'seconds' else int(v))
             for k, v in r.items()} for r in rows]
    if len({r['lives'] for r in rows}) != 1 or any(r['hp'] < rows[0]['hp'] for r in rows):
        raise ValueError('Road-bricks-road replay lost health or a life')
    if len({r['hero'] for r in rows}) != 1 or any(r['stage'] for r in rows):
        raise ValueError('Road-bricks-road replay changed hero or stage')
    if any(r['cam_y'] or r['far_y'] != 0 or r['road_y'] != 192 for r in rows):
        raise ValueError('Road-bricks-road replay moves the floor or view')
    visible = [r for r in rows if r['brick_y'] != -999]
    if len(visible) < 100 or any(r['brick_y'] != 144 for r in visible):
        raise ValueError('Road-side bricks move against the painted floor')
    for phase, height in (('brick_rest', 144), ('ground_rest', 192)):
        samples = [r for r in rows if r['phase'] == phase]
        if len(samples) < 60 or any(r['y'] != height for r in samples):
            raise ValueError(f'{phase}: wrong footing')
        if any(r['sprite_feet'] != height for r in samples[-20:]):
            raise ValueError(f'{phase}: rendered heroine sinks below her footing')
    for phase in ('onto_bricks', 'brick_jump', 'off_bricks'):
        samples = [r for r in rows if r['phase'] == phase]
        if not samples or not any(r['vy'] > 0 for r in samples):
            raise ValueError(f'{phase}: missing descent')
        if any(r['canvas_top'] < 0 or r['canvas_bottom'] > 224 or r['scb_rows'] != 4 for r in samples):
            raise ValueError(f'{phase}: heroine clipping or missing rows')
    return dict(passed=True, hardware_samples=len(rows), fixed_road_y=192, fixed_brick_y=144,
                road_bricks_road=True, heroine_aligned_on_bricks_and_ground=True)

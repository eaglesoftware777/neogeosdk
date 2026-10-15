"""Directed keyframe composition from named native drawings and integer offsets.

Every referenced combat pose has an explicit binding. Unknown poses fail;
there is no procedural-body fallback or automatic interpolation.
"""
from showcase_ink import head, torso
from showcase_arms import arm
from showcase_legs import leg
from showcase_floor import floor_head, floor_torso, bow_torso, curl_torso, inverted_torso, inverted_head

def frame(who,near='guard',far='guard',fl='plant',nl='plant',body=(0,0),
          face='focus',na=(0,0),fa=(0,0),hd=(0,0),twist=False,step=None):
    return {
      'torso':torso(who,'twist' if twist else 'guard').shifted(*body),
      'head_hair':head(who,face).shifted(body[0]+hd[0],body[1]+hd[1]),
      'near_arm':arm(who,'near_arm',near).shifted(body[0]+na[0],body[1]+na[1]),
      'far_arm':arm(who,'far_arm',far).shifted(body[0]+fa[0],body[1]+fa[1]),
      'far_leg':leg(who,'far_leg',fl,step),
      'near_leg':leg(who,'near_leg',nl,step),
    }

def common(who,name):
    if name=='stance':return frame(who)
    if name=='v2_breathe':return frame(who,body=(0,1),hd=(0,-1),na=(0,-1))
    if name.startswith('v2_step') or name.startswith('ink_step'):
        n=int(name[-1]);bob=(0,1,0,-1,0,1,0,-1)[n]
        return frame(who,body=((0,1,2,1,0,-1,-2,-1)[n] if who=='amara' else 0,bob),step=n)
    if name=='walk_a':return frame(who,body=(2,1),step=1)
    if name in ('crouch','guard_low','getup'):
        return frame(who,near='parry' if name=='guard_low' and who!='jack' else 'guard',fl='crouch',nl='crouch',body=(12,24) if name!='getup' else (6,20),face='focus',twist=True)
    if name=='guard':return frame(who,near='parry' if who!='jack' else 'guard',na=(-2,-2),far='guard',fa=(-2,0),twist=True)
    if name in ('jump','jump_b'):
        return frame(who,fl='tuck',nl='tuck',body=(0,-4),na=(0,-1 if name=='jump' else 1),fa=(0,1 if name=='jump' else -1))
    if name in ('hit','hit_b','hit_low'):
        low=name=='hit_low'
        if low and who=='jack':return frame(who,near='guard',far='low',fl='crouch',nl='crouch',body=(-6,20),hd=(-4,-1),face='hurt',twist=True)
        return frame(who,near='relax',far='guard' if name=='hit' else 'relax',fl='crouch' if low else 'gather',nl='crouch' if low else 'support',body=(-6,20) if low else (-6,3),hd=(-4,-1),face='hurt',twist=True)
    if name in ('fall','down'):
        return {'torso':floor_torso(who,name=='fall'),'head_hair':floor_head(who,name=='fall'),
                'far_leg':leg(who,'far_leg',name),'near_leg':leg(who,'near_leg',name),
                'far_arm':arm(who,'far_arm',name),'near_arm':arm(who,'near_arm',name)}
    if name=='dash':return frame(who,near='wind' if who!='kenji' else 'chamber',far='low',fl='dash',nl='dash',body=(7,6),hd=(5,-2),face='focus',twist=True)
    if name=='backdash':return frame(who,fl='plant',nl='tuck',body=(-5,1),near='guard',far='guard')
    if name in ('win','win_b'):
        return frame(who,near='raise',far='relax',face='shout' if name=='win' else 'focus',na=(0,0) if name=='win' else (1,-1),body=(0,0),hd=(-10,1),twist=True)
    if name=='lose':return frame(who,near='body' if who=='jack' else 'relax',far='low' if who=='jack' else 'relax',fl='crouch',nl='crouch',body=(4,24),face='hurt',hd=(0,3))
    if name in ('dizzy','dizzy_b'):
        return frame(who,near='relax',far='relax',fl='gather',nl='support',body=(-2,2),face='hurt',hd=(-2 if name=='dizzy' else 1,1))
    if name in ('roll','evade_0'):
        p=frame(who,fl='curl',nl='curl',body=(-6,65),near='floor',far='floor')
        p['torso']=curl_torso(who);p['near_arm']=arm(who,'near_arm','floor');p['far_arm']=arm(who,'far_arm','floor')
        return p
    return None

def kenji(name):
    q=common('kenji',name)
    if q is not None:return q
    f=lambda **kw:frame('kenji',**kw)
    if name=='bow':
        q=f(near='relax',far='relax',hd=(8,15),na=(4,3),face='calm');q['torso']=bow_torso('kenji');return q
    if name in ('jab_1',):return f(near='chamber',far='jab_wind',twist=True,body=(-1,0))
    if name=='jab_2':return f(near='chamber',far='jab',twist=True,body=(2,0))
    if name=='rev_1':return f(near='chamber',far='guard',twist=True,body=(-2,1))
    if name=='rev_2':return f(near='straight',far='guard',twist=True,body=(4,0),face='shout')
    if name in ('mae_1','j_knee','v2_j_knee_wind'):
        return f(near='guard',far='low',fl='chamber',nl='tuck' if name!='mae_1' else 'support',body=(-3,0))
    if name=='mae_2':return f(far='low',fl='front',nl='support',body=(-4,0),twist=True)
    if name=='mawashi_1':return f(near='chamber',far='parry',fl='support',nl='chamber',body=(-3,1),twist=True)
    if name=='mawashi_2':return f(near='chamber',far='low',fl='support',nl='high',body=(-5,0),twist=True)
    if name in ('v2_c_jab_wind','v2_c_jab_recover'):return f(near='guard',far='jab_wind' if name.endswith('wind') else 'jab_back',fl='crouch',nl='crouch',body=(12,24),twist=True)
    if name=='c_jab':return f(near='chamber',far='jab',fl='crouch',nl='crouch',body=(12,24),twist=True)
    if name in ('v2_c_kick_wind','v2_c_kick_recover'):return f(far='low',fl='crouch',nl='crouch',body=(10,24),na=(-2,1),twist=True)
    if name=='c_kick':return f(far='low',fl='low',nl='crouch',body=(9,24),twist=True)
    if name=='c_rise_1':return f(near='chamber',far='guard',fl='crouch',nl='crouch',body=(6,20),twist=True)
    if name=='c_rise_2':return f(near='upper',far='guard',fl='gather',nl='support',body=(0,-4),twist=True,face='shout')
    if name=='sweep_1':return f(near='parry',far='low',fl='crouch',nl='crouch',body=(8,24),twist=True)
    if name=='sweep_2':return f(near='parry',far='low',fl='crouch',nl='sweep',body=(8,24),twist=True)
    if name=='v2_j_punch_wind':return f(near='guard',far='jab_wind',fl='tuck',nl='tuck',body=(0,-4),twist=True)
    if name=='j_punch':return f(near='chamber',far='jab',fl='tuck',nl='tuck',body=(1,-4),twist=True)
    if name=='v2_j_kick_wind':return f(near='chamber',far='guard',fl='tuck',nl='chamber',body=(-3,-2),twist=True)
    if name=='j_kick':return f(near='chamber',far='low',fl='tuck',nl='side',body=(-4,-2),twist=True)
    if name=='lunge_0':return f(near='chamber',far='low',fl='dash',nl='dash',body=(5,7),hd=(2,0),twist=True)
    if name=='lunge_1':return f(near='straight',far='low',fl='dash',nl='dash',body=(10,6),hd=(2,-2),twist=True,face='shout')
    if name=='uke_0':return f(near='parry',far='low',fl='gather',nl='support',body=(-1,0),twist=True)
    if name=='uke_1':return f(near='parry',far='parry',fl='gather',nl='support',body=(-2,0),na=(0,-2),twist=True)
    if name=='uke_hit':return f(near='straight',far='parry',body=(5,1),twist=True)
    if name=='v2_uke_strike_recover':return f(near='follow',far='guard',body=(2,1),twist=True)
    if name=='axe_0':return f(near='parry',far='guard',fl='tuck',nl='chamber',body=(0,-2),twist=True)
    if name=='axe_1':return f(near='parry',far='low',fl='tuck',nl='axe_up',body=(-4,-2),twist=True)
    if name=='axe_2':return f(near='chamber',far='parry',fl='tuck',nl='axe_down',body=(0,1),twist=True,face='shout')
    if name=='v2_axe_recover':return f(near='guard',far='low',fl='gather',nl='support',body=(3,4),twist=True)
    if name=='super_0':return f(near='chamber',far='parry',fl='gather',nl='support',body=(-2,2),face='shout',twist=True)
    if name=='super_a':return f(near='straight',far='guard',fl='dash',nl='dash',body=(10,4),face='shout',twist=True)
    if name=='super_b':return f(near='hook',far='jab_wind',fl='dash',nl='dash',body=(7,5),face='shout',twist=True)
    if name=='throw_0':return f(near='reach',far='reach',body=(3,1),twist=True)
    if name=='throw_1':return f(near='reach',far='low',fl='gather',nl='support',body=(5,8),twist=True,face='shout')
    if name=='throw_2':return f(near='relax',far='reach',fl='crouch',nl='crouch',body=(8,24),twist=True)
    if name=='push_kick':return f(near='parry',far='low',fl='support',nl='side',body=(-4,0),twist=True)
    raise ValueError('Undrawn karate pose: '+name)

def jack(name):
    q=common('jack',name)
    if q is not None:return q
    f=lambda **kw:frame('jack',**kw)
    if name in ('intro','intro_b'):return f(near='guard' if name=='intro' else 'shove',far='guard',face='shout' if name=='intro_b' else 'focus',twist=True)
    if name in ('jab_1','body_1','cross_1','hook_1'):return f(near='wind',far='guard',body=(-2,1),twist=True,na=(0,0 if name=='jab_1' else 3))
    if name=='v2_jab_punch':return f(near='jab',far='guard',body=(2,0),twist=True)
    if name in ('v2_body_punch','body_2'):return f(near='body',far='guard',body=(3,4),hd=(2,-2),twist=True)
    if name=='v2_cross_punch':return f(near='cross',far='guard',body=(4,0),face='shout',twist=True)
    if name=='cross_2':return f(near='follow',far='guard',body=(3,1),twist=True)
    if name in ('v2_hook_punch','hook_2'):return f(near='hook',far='guard',body=(4,1),face='shout',twist=True)
    if name in ('v2_c_jab_wind','v2_c_body_wind','c_upper_1','c_dig_1'):return f(near='wind',far='guard',fl='crouch',nl='crouch',body=(9,24),twist=True)
    if name in ('v2_c_jab_recover','v2_c_body_recover'):return f(near='follow',far='guard',fl='crouch',nl='crouch',body=(9,24),twist=True)
    if name=='v2_c_jab_punch':return f(near='jab',far='guard',fl='crouch',nl='crouch',body=(10,24),twist=True)
    if name in ('v2_c_body_punch','v2_c_dig_punch'):return f(near='body',far='guard',fl='crouch',nl='crouch',body=(9,24),twist=True)
    if name=='v2_c_upper_punch':return f(near='upper',far='guard',fl='gather',nl='support',body=(2,6),twist=True,face='shout')
    if name in ('v2_j_jab_wind','v2_j_over_wind','v2_j_hammer_wind'):return f(near='wind',far='guard',fl='tuck',nl='tuck',body=(0,-3),twist=True)
    if name=='v2_j_jab_punch':return f(near='jab',far='guard',fl='tuck',nl='tuck',body=(0,-3),twist=True)
    if name=='v2_j_over_punch':return f(near='high',far='guard',fl='tuck',nl='tuck',body=(0,-3),twist=True)
    if name=='v2_j_hammer_punch':return f(near='body',far='low',fl='tuck',nl='tuck',body=(0,-3),twist=True)
    if name=='rush_0':return f(near='wind',far='low',fl='dash',nl='dash',body=(5,6),hd=(2,-2),twist=True)
    if name=='rush_1':return f(near='cross',far='guard',fl='dash',nl='dash',body=(9,5),hd=(1,-1),face='shout',twist=True)
    if name in ('weave_0','weave_1'):
        return f(near='guard',far='low',fl='crouch',nl='crouch',body=(7 if name=='weave_0' else 14,22),hd=(0,-2),twist=True)
    if name=='upper_0':return f(near='wind',far='guard',fl='crouch',nl='crouch',body=(6,18),twist=True)
    if name=='upper_1':return f(near='upper',far='guard',fl='tuck',nl='tuck',body=(2,-4),face='shout',twist=True)
    if name=='upper_2':return f(near='upper',far='low',fl='tuck',nl='support',body=(-1,1),na=(-3,5),twist=True)
    if name=='v2_upper_recover':return f(near='guard',far='guard',fl='gather',nl='support',body=(1,4),twist=True)
    if name=='flurry_a':return f(near='cross',far='guard',fl='dash',nl='dash',body=(9,5),twist=True,face='shout')
    if name=='flurry_b':return f(near='hook',far='jab',fl='dash',nl='dash',body=(3,5),twist=True,face='shout')
    if name=='v2_super_recover':return f(near='follow',far='low',fl='gather',nl='support',body=(2,5),twist=True)
    if name=='clinch':return f(near='shove',far='reach',body=(4,1),twist=True)
    if name=='clinch_b':return f(near='body',far='reach',fl='gather',nl='support',body=(3,9),twist=True,face='shout')
    if name=='shove':return f(near='guard',far='jab',body=(4,2),twist=True)
    raise ValueError('Undrawn boxing pose: '+name)

def acrobat(near_leg='inverted',far_leg='inverted'):
    return {'torso':inverted_torso(),'head_hair':inverted_head(),
            'far_arm':arm('amara','far_arm','floor'),'near_arm':arm('amara','near_arm','floor'),
            'far_leg':leg('amara','far_leg',far_leg),'near_leg':leg('amara','near_leg',near_leg)}

def amara(name):
    q=common('amara',name)
    if q is not None:return q
    f=lambda **kw:frame('amara',**kw)
    if name=='intro':return f(near='parry',far='guard',step=0,face='focus')
    if name=='intro_b':return f(near='guard',far='reach',step=4,body=(1,1),twist=True)
    if name=='ginga_1':return f(near='wind',far='guard',step=1,body=(-1,1),twist=True)
    if name=='ginga_2':return f(near='guard',far='slap',step=0,body=(3,0),twist=True)
    if name=='v2_ginga_slap_recover':return f(near='guard',far='reach',step=1,body=(1,1),twist=True)
    if name=='martelo_1':return f(near='parry',far='low',fl='chamber',nl='support',body=(-3,0),twist=True)
    if name=='martelo_2':return f(near='parry',far='low',fl='high',nl='support',body=(-5,0),twist=True)
    if name in ('v2_c_slap_wind','v2_c_slap_recover'):return f(near='parry',far='guard' if name.endswith('wind') else 'reach',fl='crouch',nl='crouch',body=(10,24),twist=True)
    if name=='ink_c_slap':return f(near='parry',far='slap',fl='crouch',nl='crouch',body=(10,24),twist=True)
    if name in ('v2_c_rasteira_wind','v2_c_rasteira_recover'):return f(near='parry',far='low',fl='crouch',nl='crouch',body=(9,24),twist=True)
    if name=='c_rasteira':return f(near='parry',far='low',fl='low',nl='crouch',body=(8,24),twist=True)
    if name=='v2_j_kick_wind':return f(near='parry',far='guard',fl='tuck',nl='chamber',body=(-2,-3),twist=True)
    if name=='j_kick':return f(near='guard',far='low',fl='tuck',nl='high',body=(-4,-3),twist=True)
    if name=='meia_0':return acrobat('chamber','compass')
    if name=='meia_1':return acrobat('spin','compass')
    if name=='au_0':return acrobat('chamber','inverted')
    if name=='au_1':return acrobat()
    if name=='v2_au_batido_recover':return f(near='relax',far='low',fl='crouch',nl='crouch',body=(6,24),twist=True)
    if name=='super_0':return f(near='parry',far='reach',fl='gather',nl='support',body=(-1,1),face='shout',twist=True)
    if name=='super_1':return f(near='parry',far='low',fl='support',nl='high',body=(-5,0),twist=True)
    if name=='super_2':return acrobat('spin','compass')
    if name=='super_3':return acrobat()
    if name=='v2_super_recover':return f(near='guard',far='low',fl='crouch',nl='crouch',body=(7,20),twist=True)
    if name=='throw_0':return f(near='reach',far='reach',body=(3,2),twist=True)
    if name=='throw_1':return f(near='reach',far='relax',fl='crouch',nl='sweep',body=(8,24),twist=True)
    if name=='guard_push':return f(near='slap',far='guard',body=(3,1),twist=True)
    raise ValueError('Undrawn capoeira pose: '+name)

DRAW={'kenji':kenji,'jack':jack,'amara':amara}

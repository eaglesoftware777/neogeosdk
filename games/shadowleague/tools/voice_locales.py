"""Native-language short combat lines for the first localized voice pass.

Sample keys and their ROM indices stay unchanged. Other cast members retain
existing recordings until a reviewed language/speaker pairing is available.
"""
LOCALES={}
def cast(ids,language,voice,lines):
    for id in ids.split():LOCALES[id]={'language':language,'voice':voice,'lines':dict(zip(('sp1','sp2','sp3','super'),lines))}
cast('kenji ryuhei hayato','ja','jm_kumo',['いくぞ！','そこだ！','甘い！','これで終わりだ！'])
cast('claire aisha','fr-fr','ff_siwis',['En garde !','À moi !','Prends ça !',"C'est terminé !"])
cast('amara','pt-br','pf_dora',['Vamos lá!','Agora!','Toma!','Acabou!'])
cast('lucas','pt-br','pm_alex',['Vamos lá!','Agora!','Toma!','Acabou!'])
cast('diego tomas tororojo','es','em_alex',['¡Aquí voy!','¡Arriba!','¡Toma!','¡Se acabó!'])
cast('linwei chenbao','zh','zm_yunjian',['看招！','破！','来了！','结束了！'])
cast('xiaomei','zh','zf_xiaobei',['看招！','破！','来了！','结束了！'])
cast('arun','hi','hm_omega',['तैयार हो!','आगे!','लो!','अब समाप्त!'])
cast('kiran','en-gb','bf_emma',['Here I come!','Up you go!','Take this!','It ends now!'])

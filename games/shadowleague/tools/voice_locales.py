"""Fighters' lines in their own language: specials, super, heavy blow, throw and win.

Each localized fighter gets seven lines: the three specials and the super
(sp1, sp2, sp3, super: the move names, as they would be called out at home),
the heavy blow, the throw and the win (the first line of their quote). The
short grunts -- the light blow's shout, being hurt, the knock-out -- stay the
same kind for every fighter (slf_voices.SHOUTS), in their own speaker.

Kokoro (voices_localized.py) speaks Japanese, Mandarin, French, Brazilian
Portuguese, Spanish, Hindi and British English; Piper (voices_localized_piper.py)
speaks Russian, Ukrainian and a second French pair. A fighter whose language
has no voice with clear terms keeps English (slf_voices): the sources and
their licences are in assets/authored/voices/voice-sources-sdk177.md.
Sample keys and numbers stay those of slf_voices.plan().
"""
LOCALES = {}
KEYS = ('sp1', 'sp2', 'sp3', 'super', 'heavy', 'throw', 'win')


def cast(ids, language, voice, lines, engine='kokoro', model=None, speaker=None):
    for fid in ids.split():
        LOCALES[fid] = {'language': language, 'voice': voice, 'engine': engine, 'model': model,
                        'speaker': speaker, 'lines': dict(zip(KEYS, lines))}


# Japanese, in kana: the reading of each word is then the one meant
cast('kenji', 'ja', 'jm_kumo', ['かかとおとし！', 'れっせんづき！', 'しんうけ！', 'いちげきひっさつ！', 'せいやっ！', 'そりゃあ！',
                                'まけたゆかに、れいをしろ。'])
cast('ryuhei', 'ja', 'jm_kumo', ['せおいなげ！', 'おおそとがり！', 'かえしわざ！', 'ひっさつのなげ！', 'でやぁっ！', 'いっぽん！',
                                 'きれいななげ。いっぽんだ。'])
cast('hayato', 'ja', 'jm_kumo', ['いちげきで、おわりだ！', 'こりゅうのうけ！', 'いにしえのはらい！', 'しんのごくい！', 'ふんっ！', 'まいる！',
                                 'からだは、はやい。'])
# Mandarin
cast('linwei', 'zh', 'zm_yunjian', ['闪电拳脚！', '擒龙手！', '扫影腿！', '散打风暴！', '喝！', '摔！', '踢，打，摔。完事。'])
cast('chenbao', 'zh', 'zm_yunxi', ['铁山靠！', '冲天肘！', '震地！', '八极爆发！', '哈！', '倒！', '一步，一击。'])
cast('xiaomei', 'zh', 'zf_xiaobei', ['连环拳！', '拍手！', '正踢！', '咏春连打！', '嘿！', '走！', '短，直，简单。'])
# French
cast('claire', 'fr-fr', 'ff_siwis', ['Fouetté chassé !', 'Revers spirale !', 'Piétinement !', "Ballet d'acier !",
                                     'Tiens !', 'Dehors !', "L'élégance n'est pas de la mollesse."])
cast('aisha', 'fr-fr', 'ff_siwis', ['La lance !', 'Crochet !', 'Pas du guerrier !', 'Écrasement Dambe !',
                                    'Prends ça !', 'Par terre !', 'Mes poings étaient bandés pour la guerre.'])
cast('malik', 'fr', 'pierre', ['Charge du Sahara !', 'Paume du tonnerre !', 'Le lion frappe !', 'Avalanche de Dakar !',
                               'Tiens !', 'Dans le sable !', 'Le sable se souvient des champions.'],
     engine='piper', model='fr_FR-upmc-medium', speaker=1)
cast('idris', 'fr', 'pierre', ["Crochet de l'Atlas !", 'Croissant du désert !', 'Genou !', 'Fureur de Casablanca !',
                               'Prends ça !', 'Par terre !', 'La vue de ce toit est belle.'],
     engine='piper', model='fr_FR-upmc-medium', speaker=1)
cast('leila', 'fr-fr', 'ff_siwis', ['Direct !', 'Genou du Sahara !', 'Crochet méditerranéen !', 'Éclipse de Carthage !',
                                    'Trop lent !', 'Dégage !', 'Trop lent pour la brise de mer.'])
# Brazilian Portuguese
cast('amara', 'pt-br', 'pf_dora', ['Meia lua de compasso!', 'Aú batido!', 'Dança da tempestade!', 'Dança da tempestade!',
                                   'Toma!', 'Vai!', 'Você lutou contra a música, não contra mim.'])
cast('lucas', 'pt-br', 'pm_alex', ['Puxando pra guarda!', 'Chave de braço!', 'Raspagem borboleta!', 'Finalização!',
                                   'Toma essa!', 'Peguei!', 'Bater não é vergonha.'])
# Spanish
cast('diego', 'es', 'em_alex', ['¡Huracanrana!', '¡Clavado torpedo!', '¡Tornado mexicano!', '¡Eclipse azteca!',
                                '¡Ándale!', '¡Vámonos!', '¡Toda la arena grita mi nombre!'])
cast('tomas', 'es', 'em_santa', ['¡Puño del toro!', '¡Patada trasera!', '¡Hacha de fuego!', '¡Tempestad ibérica!',
                                 '¡Olé!', '¡Fuera!', 'Barcelona vino a ver un espectáculo.'])
cast('tororojo', 'es', 'em_alex', ['¡Prensa del toro!', '¡Bomba del toro!', '¡Toro volador!', '¡Rojo Megaplex!',
                                   '¡Toro!', '¡Al suelo!', '¡El Toro Rojo nunca se detiene!'])
# Hindi (Arun's Kalaripayattu is from Kerala: Hindi stands in for Malayalam;
# Zara's Urdu is spoken as Hindustani)
cast('arun', 'hi', 'hm_omega', ['नाग प्रहार!', 'मंदिर की छलांग!', 'चक्र वार!', 'केरल का नृत्य!', 'हा!', 'गिरो!',
                                'हर वार का एक निशाना होता है।'])
cast('zara', 'hi', 'hf_alpha', ['कुश्ती का उठान!', 'गदा का वार!', 'पकड़ो और फेंको!', 'लाहौर का तूफ़ान!', 'शाबाश!',
                                'मिट्टी में!', 'अखाड़े की मिट्टी सच्ची है।'])
# British English
cast('kiran', 'en-gb', 'bf_emma', ['Cross-knee flow!', 'London high!', 'Pivot low!', "Queen's knockout!",
                                   'Have it!', 'Down you go, mate!', "London's cold, mate."])
# Russian, Ukrainian
cast('aleksei', 'ru', 'dmitri', ['Бросок с шагом!', 'Удар с шага!', 'Рычаг локтя!', 'Титан степи!', 'Получай!',
                                 'Через себя!', 'В горах побеждает холод.'],
     engine='piper', model='ru_RU-dmitri-medium')
cast('viktor', 'uk', 'mykyta', ['Тактичний прохід!', 'Підсічка!', 'Залізна контратака!', 'Удар командос!', 'Тримай!',
                                'На землю!', 'Цей склад — мій дім.'],
     engine='piper', model='uk_UA-mykyta-high')

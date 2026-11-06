"""Shadow League roster art spec: original character designs and per-fighter pose notes."""

# id: (pronoun, height cm, role, design paragraph, short reminder of look,
#      guard, specials [(name, wind-up, strike, follow)], super [3 beats], win, intro)
R = {}


def f(id, he, cm, role, design, short, guard, sp, sup, win, intro, family):
    R[id] = dict(id=id, he=he, cm=cm, role=role, design=design, short=short, guard=guard,
                 sp=sp, sup=sup, win=win, intro=intro, family=family)


f('kenji', 'he', 178, 'karate fighter',
  "KENJI ARATA, 27, Japanese traditional karate fighter, lean and hard-muscled. Short black hair shaved at the sides with a longer swept-back top and loose strands over the forehead, thin pale scar through his left eyebrow, narrow focused eyes. Heavy deep-indigo hand-dyed karate gi with faded patches, sleeves rolled above the elbows, worn black belt with frayed ends, a crimson-red cord tied in an X across his back and over his shoulders, long indigo gi trousers with frayed hems, white cloth wraps on both hands and wrists, barefoot. No headband.",
  "indigo gi with rolled sleeves, black belt, red cord across the back, white hand wraps, barefoot, short black swept hair",
  "classic karate fighting stance, side-on, front fist forward at chin height, rear fist chambered at the hip, knees bent",
  [("RESSEN ZUKI lunging punch", "dropping low, rear fist chambered, coiled to lunge", "lunging forward with a full-length straight punch, front knee deeply bent, rear leg stretched", "recovering from the lunge, punching arm drawn back"),
   ("KAKATO OTOSHI axe kick", "leaping up raising one straight leg above his head", "bringing the heel crashing straight down in a vertical axe kick", "landing from the axe kick, crouched"),
   ("KOKORO UKE parry", "sweeping a circular open-hand block in front of his face", "counter-striking with a sharp reverse punch out of the parry", "")],
  ["focusing, both fists pulled to the hips, breathing out", "launching forward in a blur of straight punches, fist fully extended", "finishing with a huge rising reverse punch, whole body stretched upward"],
  "standing calmly, bowing his head with one fist pressed into his open palm",
  "tightening his black belt with both hands", 'karate')

f('somchai', 'he', 172, 'Muay Thai fighter',
  "SOMCHAI RATTAN, 29, Thai Muay Thai fighter, wiry and sinewy, ribs and abdominal muscles visible. Short black hair with a shaved undercut, a sacred braided mongkhon headband of red and gold cord, a red pra jiad armband tied around his left biceps, thin traditional ink line tattoos over his shoulder blades. Bare chest. Deep burgundy satin Muay Thai shorts with gold trim and gold side panels, white hand wraps, white ankle supports, barefoot.",
  "bare chest, burgundy and gold Muay Thai shorts, red-gold mongkhon headband, red armband, white wraps, barefoot",
  "tall Muay Thai guard, upright, both forearms high framing the face, lead knee lifted slightly on the toes",
  [("TIGER ELBOW", "turning the shoulder, cocking the elbow high", "slashing a horizontal elbow strike forward, stepping in", "recovering, arm folding back to guard"),
   ("RISING KNEE", "grabbing forward with both hands, rear knee chambered", "flying straight up with a knee strike driven above his head height", "dropping back down, knee lowering"),
   ("MEKONG SWEEP", "spinning low on one heel", "a long low spinning sweep at ankle height", "")],
  ["raising the knee and both elbows in a defiant pose", "a flurry of elbows, upper body twisting, elbow driven forward", "a soaring jumping knee strike, body arched"],
  "kneeling on one knee with his palms pressed together in a respectful prayer bow",
  "performing a slow ritual dance, one knee raised, arms spread like wings", 'thai')

f('amara', 'she', 172, 'capoeira fighter',
  "AMARA SILVA, 24, Afro-Brazilian capoeira fighter from Salvador, athletic with long legs and toned shoulders. Dark brown skin, long black box braids gathered into a high thick bun with a few loose braids and small gold beads, small gold hoop earrings, confident eyes. Cropped emerald-green sleeveless athletic top with yellow trim, loose white capoeira trousers gathered at the ankles, a braided green and yellow corded belt knotted at the hip with hanging tassels, thin white wrap around one ankle, barefoot.",
  "cropped emerald-green top with yellow trim, loose white trousers, green-yellow cord belt with tassels, box braids in a high bun, barefoot",
  "low swaying ginga stance, legs wide, one arm guarding her face and the other hand low, rocking side to side",
  [("MEIA LUA DE COMPASSO", "planting both hands toward the ground, body turning", "a huge spinning heel kick with the leg sweeping in a high arc while her hands are on the floor", "coming back up out of the spin"),
   ("AU BATIDO", "tipping sideways into a cartwheel", "kicking straight up from a one-handed cartwheel, legs split vertically", "landing back into the ginga"),
   ("ARMADA", "stepping across, turning her back", "a spinning outside crescent kick at head height", "")],
  ["springing into a handstand", "upside-down in a whirling storm of kicks, legs spread", "landing a final flying spinning kick, body horizontal in the air"],
  "dancing a light celebratory ginga with one arm raised high",
  "clapping her hands overhead to the rhythm, smiling", 'flow')

f('jack', 'he', 183, 'boxer',
  "JACK MERCER, 31, American professional boxer, heavy-middleweight, broad shoulders and thick forearms but realistic proportions. Close-cropped copper-red hair, light freckled skin, a once-broken nose, a short taped cut over his right eyebrow, stubble, heavy jaw. Faded charcoal-black sleeveless tank top, navy-blue satin boxing trunks with a wide oxblood-red waistband and a thin white stripe down each side, oxblood-red leather boxing gloves with white laces and white wrist tape, navy and white high lace-up boxing boots with white socks.",
  "black tank top, navy trunks with red waistband and white stripes, red boxing gloves, navy-white high boots, copper-red crew cut",
  "tight high boxing guard, chin tucked, gloves by the cheeks, elbows in, knees bent",
  [("BROOKLYN RUSH dashing hook", "dipping low and pushing off the back foot", "rushing forward with a long body-weight straight right, rear foot trailing", "rolling the shoulders back into guard"),
   ("RISING BELL uppercut", "crouching, gloves low, loading the legs", "exploding upward with a leaping uppercut, glove high above his head, feet off the floor", "dropping back down"),
   ("WEAVE", "bobbing sharply under an imaginary punch, upper body ducked", "springing out of the weave with a crushing haymaker hook", "")],
  ["a feint with the shoulders, eyes burning", "a blistering combination, right hook swung fully around", "a final colossal uppercut, glove thrust to the sky"],
  "raising one glove high in triumph, the other on his hip",
  "bumping his gloves together, rolling his neck", 'boxing')

f('claire', 'she', 170, 'savate fighter',
  "CLAIRE MOREAU, 26, French savate fighter, slim and athletic with long elegant legs. Auburn hair in a short asymmetrical bob tucked behind one ear, sharp green eyes, a thin mole under one eye. A fitted cobalt-blue sleeveless one-piece savate suit top with a high collar and ivory piping, long ivory savate trousers, black pointed-toe savate boots with laces, black fingerless training gloves.",
  "cobalt-blue high-collar sleeveless top with ivory piping, ivory trousers, black pointed savate boots, black fingerless gloves, auburn bob",
  "light, upright savate guard, side-on, hands loose at chin level, weight on the back leg, front foot on its toes",
  [("FOUETTE CHASSE", "lifting the front knee high, body leaning away", "a whipping roundhouse kick snapping out at head height with the toe of the boot", "retracting the kicking leg"),
   ("REVERS SPIRALE", "turning her back, rising on the supporting foot", "a spinning reverse hook kick sweeping upward at head height", "landing out of the spin"),
   ("PIETINEMENT", "stepping forward with a stomping skip", "a low piston side kick driving the sole into the knee", "")],
  ["a small elegant salute", "a long chain of whipping kicks, leg high", "a soaring spinning kick, coat of motion around her"],
  "brushing her hair behind her ear with one hand, the other hand on her hip",
  "taking a fencer's salute, then sliding into guard", 'savate')

f('viktor', 'he', 185, 'combat sambo fighter',
  "VIKTOR ORLOV, 33, Ukrainian combat sambo fighter, stocky and powerful, thick neck and wide back. Close-cropped fair hair, short dark-blond beard, heavy brow, crooked nose, grey eyes. A heavy red sambo jacket (kurtka) with short sleeves and shoulder epaulette straps, black belt through the loops, navy sambo shorts, red and navy lace-up wrestling boots, taped fingers.",
  "red short-sleeved sambo jacket, black belt, navy shorts, red-navy wrestling boots, fair crew cut and short beard",
  "low wide grappling guard, hands open and forward reaching to grab, chin down",
  [("TACTICAL TACKLE", "dropping his level, arms reaching", "shooting forward in a low double-leg tackle, body almost horizontal", "rising from the tackle"),
   ("IRON COUNTER", "bracing with both forearms raised", "catching and driving a short brutal elbow upward", "resetting his guard"),
   ("LEG REAP", "stepping deep and hooking a leg", "reaping low with a sweeping outside leg kick", "")],
  ["seizing forward with both hands", "lifting overhead, arms raised high, back arched", "slamming down, kneeling with both fists driven into the floor"],
  "folding his arms across his chest, chin raised",
  "cracking his knuckles, then rolling his shoulders", 'grapple')

f('linwei', 'he', 176, 'sanda fighter',
  "LIN WEI, 24, Chinese sanda fighter, lean and quick. Swept black fringe over sharp narrow eyes, high cheekbones. A black sleeveless mandarin-collar fighting shirt with teal piping and teal frog buttons, black loose trousers with a teal sash, teal hand wraps, white ankle wraps, black kung fu slippers.",
  "black sleeveless mandarin shirt with teal frog buttons, black trousers, teal sash, teal hand wraps, black slippers, swept black fringe",
  "light springy sanda guard, lead hand open and extended, rear hand by the chin, on the balls of his feet",
  [("LIGHTNING JAB-KICK", "skipping forward on the front foot", "a lightning-fast side kick straight out at chest height", "retracting the leg into guard"),
   ("DRAGON CATCH", "low with both palms open, coiled", "springing upward with a rising palm strike above his head", "landing in guard"),
   ("SWEEPING SHADOW", "dropping into a low squat", "a full spinning low sweep on one hand, leg stretched", "")],
  ["sliding back with palms forward", "a storm of kicks, leg flashing in a high side kick", "a leaping spinning kick high in the air"],
  "standing with one palm raised before his chest and the other fist behind his back",
  "spinning a fast kata flourish of palms", 'palm')

f('batu', 'he', 192, 'Mongolian wrestler',
  "BATU TEMUR, 35, Mongolian bokh wrestler, massive heavyweight with a barrel chest, thick arms and a strong belly. Shaved head, thick black beard, small fierce eyes, a scar across the cheek. An open-front dark-blue silk wrestling vest with long sleeves and orange trim that leaves his chest bare, tight brown leather wrestling briefs over brown shorts, tall ornate leather boots with upturned toes in brown and orange.",
  "open-chest long-sleeved dark-blue vest with orange trim, brown leather wrestling shorts, tall brown boots with upturned toes, shaved head, thick black beard",
  "wide low heavyweight wrestling crouch, arms spread wide like an eagle, leaning forward",
  [("BULL TACKLE", "lowering his head and shoulder", "charging forward with a shoulder tackle, body leaning hard", "skidding to a stop"),
   ("IRON BEAR HUG", "spreading both arms wide", "lunging forward to crush with a bear hug, arms closing", "releasing"),
   ("STEPPE TORNADO SLAM", "grabbing high with both hands", "spinning with arms locked in a tornado throw", "")],
  ["spreading his arms in the eagle dance", "lifting an opponent-sized weight high above his head", "slamming down onto one knee, ground shaking"],
  "performing the eagle dance, arms spread like wings, one leg raised",
  "slapping his thighs and spreading his arms wide", 'heavy')

f('minjun', 'he', 180, 'taekwondo fighter',
  "MIN-JUN PARK, 22, South Korean taekwondo fighter with very long legs and a slender athletic build. Dark layered hair tied in a short ponytail with loose bangs, cool calm eyes. An ivory V-neck dobok jacket with a wide deep-blue collar, worn open over a fitted blue compression shirt, a black belt with gold stitching, ivory dobok trousers, barefoot.",
  "ivory dobok with deep-blue V collar open over a blue shirt, black belt, ivory trousers, short dark ponytail, barefoot",
  "bouncy side-on taekwondo stance, hands low and relaxed, front leg light, ready to kick",
  [("FLYING DRAGON KICK", "crouching, launching off the floor", "flying forward in a horizontal jumping side kick, leg fully extended", "landing lightly"),
   ("TWIN CRESCENT KICKS", "pivoting on the supporting foot", "a rising crescent kick sweeping vertically above his head", "lowering the leg"),
   ("BAEKDU AXE KICK", "raising one leg straight up past his head", "chopping the heel straight down in an axe kick", "")],
  ["leaping high, knees tucked", "a mid-air flurry, leg snapping out", "a final towering spinning kick in the air"],
  "standing straight with his hands clasped behind his back, chin up",
  "tying his belt with a sharp tug, then bouncing into stance", 'taekwondo')

f('malik', 'he', 196, 'laamb wrestler',
  "MALIK KANE, 30, Senegalese laamb wrestler, huge and muscular with broad shoulders. Very dark skin, shaved head, short beard, calm heavy-lidded eyes. Bare chest. A traditional gold-yellow patterned loincloth wrap (ngemb) over black shorts, small leather gris-gris amulets tied around his biceps and waist on cords, barefoot.",
  "bare chest, gold-yellow patterned loincloth wrap over black shorts, leather amulets on arms and waist, shaved head, barefoot",
  "low wide wrestling stance, heavy, arms reaching forward with open palms",
  [("SAHARA CHARGE", "dropping his shoulder", "a thundering charging shoulder ram", "slowing, upright"),
   ("THUNDER PALM", "rearing back with one hand high", "a downward hammering open-palm slap", "recovering"),
   ("LION SLAM", "grabbing forward and low", "lifting and slamming with a twisting hip throw", "")],
  ["beating his chest with both palms", "sweeping a huge lifting throw overhead", "crashing down with both palms flat to the floor"],
  "raising both arms high, chest out, roaring",
  "throwing sand from his palm over his shoulder", 'heavy')

f('idris', 'he', 179, 'kickboxer',
  "IDRIS ZAHRA, 27, Moroccan kickboxer, athletic and sharp. Tan skin, dark wavy hair in a high fade, short trimmed beard, hawk-like eyes. A crimson sleeveless hooded training vest with the hood down, ivory kickboxing shorts with a crimson waistband, black hand wraps, ivory ankle guards, barefoot.",
  "crimson sleeveless hooded vest, ivory shorts with crimson waistband, black hand wraps, ivory ankle guards, wavy high-fade hair, barefoot",
  "sharp kickboxing guard, fists high, elbows tight, slight forward lean, light on his feet",
  [("ATLAS HOOK", "stepping in with a shoulder roll", "a crushing lead hook swung with his whole body", "recovering to guard"),
   ("DESERT CRESCENT", "lifting the leg in a crescent", "a high rising roundhouse kick above his head", "lowering the leg"),
   ("STEPPING KNEE", "stepping forward grabbing", "a driving knee strike to the body", "")],
  ["shuffling his feet in a quick feint", "a five-punch combination, rear straight fully extended", "a final leaping switch kick, leg high"],
  "pointing forward with a cocky smile",
  "shadowboxing a fast combination", 'kickbox')

f('arun', 'he', 172, 'kalaripayattu fighter',
  "ARUN DEV, 31, Indian kalaripayattu master, lean, supple and flexible with long sinewy muscles. Brown skin, long black hair tied in a topknot to one side, thin moustache, intense eyes, a small sandalwood mark on the forehead. Bare chest with an oiled sheen, a saffron knee-length wrapped training cloth with a red waist sash and gold border, brass rings on his upper arms, barefoot.",
  "bare oiled chest, saffron knee-length wrap with red waist sash and gold border, brass arm rings, black side topknot, barefoot",
  "very low animal stance (lion pose), knees deeply bent, one hand forward like a claw, the other high",
  [("SERPENT STRIKE", "coiling low like a snake", "lunging forward with a fingertip spear strike, body stretched long", "coiling back"),
   ("TEMPLE LEAP", "crouching, gathering", "a high leaping kick, one knee tucked and the other leg extended upward", "landing low"),
   ("COIL SWEEP", "spinning down to the floor", "a low circular sweep on one hand", "")],
  ["holding the elephant posture, arms up", "a whirl of strikes, body spinning low", "a final leaping strike in the air, arms spread"],
  "folding into a deep bow with palms touching the floor",
  "stretching into a high standing split, then settling into the lion stance", 'low')

f('ryuhei', 'he', 180, 'judoka',
  "RYUHEI GENDA, 45, Japanese judo master, broad and strong, solid heavy shoulders. Grey-streaked black hair tied back in a short tail, heavy brows, cauliflower ears, a stern weathered face with deep lines. A thick navy-blue judogi with reinforced lapels, a worn red-and-white panelled master's belt, barefoot.",
  "thick navy-blue judogi, red-and-white panelled belt, grey-streaked tied-back hair, barefoot",
  "upright judo stance, both hands forward ready to grip lapel and sleeve, knees slightly bent",
  [("SEOI NAGE", "turning in, gripping an imaginary sleeve", "a shoulder throw, back turned, hip low, arms hauling over his shoulder", "standing from the throw"),
   ("DEFENSIVE COUNTER", "bracing with hands raised to catch", "a sharp palm strike counter out of the catch", "resetting"),
   ("OSOTO GARI", "stepping beside, reaching for the collar", "reaping the leg backward with a high outer reap", "")],
  ["gripping forward with both hands", "turning under for a massive hip throw", "finishing on one knee, both hands driving downward"],
  "standing tall, adjusting his lapels with both hands, eyes closed",
  "bowing formally, hands sliding down to his knees", 'grapple')

f('diego', 'he', 178, 'luchador',
  "DIEGO CRUZ, 28, Mexican luchador, athletic and acrobatic, lean defined muscles. A full black wrestling mask with electric-blue jagged eye patterns and silver trim, laced at the back, short hair tail out the back. Bare torso. Black wrestling tights with blue side stripes, black elbow pads, blue and black lace-up wrestling boots, black wristbands.",
  "black mask with electric-blue eye pattern, bare torso, black tights with blue stripes, blue-black wrestling boots, black elbow pads",
  "springy wrestling stance, arms open and wide, bouncing, chest forward",
  [("HURRICANRANA", "running forward, leaping", "a flying scissor leap with legs spread forward in the air", "landing in a crouch"),
   ("MEXICAN TORNADO", "spinning upward off one foot", "a rising spinning wheel kick in the air", "landing"),
   ("TORPEDO DIVE", "diving forward head first", "a horizontal flying head-butt dive, arms tucked back", "")],
  ["pointing to the sky with both hands", "flipping backward in a moonsault, body arched", "a crushing flying splash, body spread wide"],
  "standing on one boot with both fists raised in a double flex",
  "leaping off the ropes and posing with arms wide", 'lucha')

f('niran', 'he', 174, 'Muay Lao fighter',
  "NIRAN VONG, 26, Lao Muay Lao fighter, lean and long-limbed. Dark short spiky hair, a white cloth headband, dark tan skin, a scar on the chin, sharp eyes. Bare chest. Navy satin shorts with ivory trim, hands and forearms bound in traditional hemp-rope wraps, ivory cord armbands on both biceps, barefoot.",
  "bare chest, navy shorts with ivory trim, white headband, hemp-rope bound hands and forearms, ivory armbands, barefoot",
  "upright Muay Lao guard, rope-wrapped forearms high, lead knee raised to check",
  [("SPINNING BACK ELBOW", "turning on the lead foot, back turned", "a spinning back elbow whipped around at head height", "recovering"),
   ("SPEAR KNEE", "leaping up, both hands reaching", "a flying knee driven straight forward and upward", "landing"),
   ("MEKONG RIVER PARRY", "sweeping a forearm across his face", "countering with a downward diagonal elbow", "")],
  ["raising both rope-bound fists overhead", "a barrage of elbows, shoulders twisting", "a soaring flying knee, body arched back"],
  "kneeling and touching his forehead with both rope-bound hands",
  "slowly circling his arms in a ritual dance", 'thai')

f('chenbao', 'he', 176, 'bajiquan fighter',
  "CHEN BAO, 38, Chinese bajiquan fighter, stocky and dense with a powerful core. Short black hair combed back with shaved sides, a small goatee, heavy brows. A black sleeveless mandarin jacket with ivory frog buttons, an ivory waist sash, wide black trousers wrapped at the shins with ivory leg bindings, black cloth shoes.",
  "black sleeveless mandarin jacket with ivory frog buttons, ivory sash, wide black trousers with ivory shin bindings, black shoes, slicked hair, goatee",
  "low horse stance, compact, elbows bent and forward, fists close",
  [("IRON SHOULDER", "stamping forward, turning sideways", "a stamping shoulder ram with the full back, sideways", "rising out of the stance"),
   ("HEAVEN ELBOW", "sinking low", "a rising elbow driven straight upward above his head", "recovering"),
   ("GROUND RUPTURE", "raising one foot high", "a thundering stomp down with a downward palm", "")],
  ["stamping hard, fists at the hips", "a crushing double palm thrust forward", "a final shoulder ram, back turned, ground cracking"],
  "standing in horse stance, saluting with fist in palm",
  "stamping three times in a short form", 'elbow')

f('aisha', 'she', 172, 'dambe boxer',
  "AISHA DIALLO, 25, Guinean dambe boxer, athletic and strong with defined arms. Deep brown skin, braided hair in a high crown bun, small scar on the lip, fierce eyes. An ivory sleeveless sports top, rust-red shorts, her right fist wrapped thickly in rust and ivory cloth and cord (the spear hand), left hand open with a cord bracelet, a cord tied around her lead knee, barefoot.",
  "ivory sleeveless top, rust-red shorts, thick cloth-wrapped right fist, open left hand, braided crown bun, barefoot",
  "dambe stance, open left hand extended forward to measure, wrapped right fist cocked high behind the head",
  [("THE SPEAR", "drawing the wrapped fist far back", "a crashing overhand strike with the wrapped fist, stepping in", "recovering"),
   ("WARRIOR STEP", "stepping high", "a rising lead kick under the guard", "landing"),
   ("LEAD HOOK", "pawing with the open hand", "a whipping hook from the wrapped fist", "")],
  ["holding the wrapped fist up proudly", "a hammering rain of overhand strikes", "a final leaping overhand blow"],
  "holding the wrapped fist high and shouting",
  "rapping the wrapped fist against her chest", 'boxing')

f('kiran', 'she', 168, 'kickboxer',
  "KIRAN SHAH, 23, British-Indian kickboxer, compact and quick. Brown skin, a long black braid down her back, a small nose stud, focused dark eyes. A black cropped zip training jacket with violet panels and sleeves pushed up, black kickboxing shorts with violet stripes, violet hand wraps, black shin guards, barefoot.",
  "black cropped zip jacket with violet panels, black shorts with violet stripes, violet hand wraps, black shin guards, long black braid, barefoot",
  "bouncy kickboxing guard, fists high, rear foot on toes",
  [("CROSS-KNEE FLOW", "stepping in with a cross", "a jumping switch knee to the head", "landing"),
   ("LONDON HIGH", "pivoting on the front foot", "a whipping head-height roundhouse kick", "retracting"),
   ("PIVOT LOW", "stepping out to the side", "a sharp low leg kick to the calf", "")],
  ["a quick feint", "a blur of punches and knees", "a leaping spinning kick"],
  "punching the air twice and grinning",
  "flicking her braid back over her shoulder", 'kickbox')

f('elena', 'she', 173, 'wrestler',
  "ELENA PETROVA, 27, Bulgarian sport wrestler, strong athletic build, broad shoulders and powerful thighs. Blonde hair in a tight French braid, pale skin, determined grey eyes. A cobalt-blue wrestling singlet with ivory side panels, ivory knee pads, cobalt high-top wrestling shoes.",
  "cobalt-blue singlet with ivory side panels, ivory knee pads, cobalt wrestling shoes, blonde French braid",
  "low square wrestling stance, hands forward, head up, ready to shoot",
  [("DOUBLE LEG BLAST", "dropping level", "shooting forward in a fast double-leg takedown, almost horizontal", "rising"),
   ("ANKLE PICK", "snapping a hand down low", "a low lunge grabbing at the ankles", "recovering"),
   ("SUPLEX RUSH", "locking her arms forward", "arching backward into a bridging suplex", "")],
  ["clapping her hands together", "lifting high with a waist lock", "bridging into a huge backward suplex"],
  "raising one arm, fist clenched, the other on her hip",
  "slapping her thighs and stepping into stance", 'grapple')

f('fahri', 'he', 188, 'oil wrestler',
  "FAHRI YILMAZ, 34, Turkish oil wrestler, heavy and broad with thick oiled muscles. Shaved head, a thick black handlebar moustache, olive skin, proud eyes. Bare oiled torso with a sheen. Heavy black leather knee-length kispet wrestling breeches with brown stitching and a laced waist, barefoot.",
  "bare oiled torso, black leather knee-length wrestling breeches, shaved head, thick black moustache, barefoot",
  "wide heavy stance, leaning forward, arms reaching low to grab the waist",
  [("KISBET GRIP", "reaching low with both hands", "plunging a hand down to grip and haul", "standing up"),
   ("ANATOLIAN BEAR", "raising both arms", "a crushing upward bear hug lift", "releasing"),
   ("BULL TACKLE", "lowering his shoulder", "a charging shoulder tackle", "")],
  ["slapping his knees", "heaving a huge lift overhead", "slamming down on both knees"],
  "raising both arms, slapping his chest",
  "slapping his knees three times and spreading his arms", 'heavy')

f('maya', 'she', 163, 'panantukan fighter',
  "MAYA SANTOS, 26, Filipina panantukan street fighter, compact and wiry. Black tousled pixie-cut hair, brown skin, a small scar on her chin, sharp eyes. A black sleeveless tactical vest with teal panels over a dark grey tee with cut sleeves, dark grey cargo trousers, black fingerless gloves, teal and white trainers.",
  "black tactical vest with teal panels, cut-sleeve grey tee, dark grey cargo trousers, black fingerless gloves, teal-white trainers, black pixie hair",
  "compact elbow-ready guard, forearms crossed high, chin tucked",
  [("MANILA ELBOW", "stepping in sideways", "a spinning elbow strike to the head", "recovering"),
   ("LOW TRAP", "parrying downward", "a low stepping knee to the thigh", "resetting"),
   ("BICEP DESTRUCTION", "catching with crossed forearms", "a short hammering elbow downward", "")],
  ["crossing forearms", "a rapid storm of elbows and hand strikes", "a final leaping elbow"],
  "rolling her shoulders and smirking, hands in her vest pockets",
  "cracking her neck and raising her forearms", 'elbow')

f('aditya', 'he', 170, 'pencak silat fighter',
  "ADITYA SURYA, 30, Indonesian pencak silat fighter, lean and supple. Brown skin, black hair under a dark batik head cloth tied in a knot, a thin moustache, calm eyes. A black silat uniform jacket with gold trim at the collar and cuffs, a gold-brown batik cloth wrapped around the waist over black loose trousers, barefoot.",
  "black silat jacket with gold trim, gold-brown batik waist cloth, black loose trousers, batik head cloth, barefoot",
  "very low sweeping silat stance, one leg folded under, hands weaving like a tiger's claws",
  [("TIGER POUNCE", "crouching low like a cat", "pouncing forward with both claw hands", "landing low"),
   ("SILAT PARRY", "weaving both hands in a circle", "trapping and striking with a rising palm", "resetting"),
   ("HARIMAU SWEEP", "dropping to the floor", "a scissoring low sweep with both legs", "")],
  ["weaving a flowing hand form", "a whirlwind of claw strikes", "a leaping double kick"],
  "standing with palms pressed together at the chest, eyes closed",
  "performing a slow flowing salutation", 'low')

f('sokha', 'he', 172, 'Kun Khmer fighter',
  "SOKHA VANN, 25, Cambodian Kun Khmer fighter, lean and tough. Brown skin, short black hair, a fierce grin, a small scar over one eye. An orange sleeveless jersey top, navy shorts with orange trim, a red and white checkered krama scarf tied around his waist, cord armbands on both arms, white hand wraps, barefoot.",
  "orange sleeveless jersey, navy shorts with orange trim, red-white checkered scarf at the waist, cord armbands, white wraps, barefoot",
  "tall Khmer guard, forearms high, elbows forward, lead knee up",
  [("LOW CHOP KICK", "stepping forward", "a chopping low kick into the thigh", "resetting"),
   ("ANGKOR RISING", "crouching with arms up", "a leaping uppercut elbow straight up", "landing"),
   ("SPINNING ELBOW", "turning his back", "a spinning back elbow at head height", "")],
  ["raising his elbows", "a hammering flurry of elbows and knees", "a soaring flying knee"],
  "raising one fist and pointing to the crowd",
  "dancing a short ritual step with his arms raised", 'thai')

f('lucas', 'he', 181, 'jiu-jitsu fighter',
  "LUCAS FERREIRA, 29, Brazilian jiu-jitsu fighter, solid and muscular. Tan skin, curly dark hair, short beard, cauliflower ears, relaxed confident eyes. A forest-green jiu-jitsu gi with heavy collar and black belt with frayed ends, gi trousers, barefoot.",
  "forest-green jiu-jitsu gi with heavy collar, black belt, curly dark hair, short beard, barefoot",
  "low grappling stance, hands forward and open, ready to pull guard",
  [("GUARD PULL", "reaching forward", "diving forward to wrap with legs and arms", "rolling up"),
   ("ARMBAR SNAP", "grabbing an arm", "leaping into a flying armbar, legs scissoring", "landing"),
   ("BUTTERFLY SWEEP", "sitting down fast", "hooking and lifting with a butterfly sweep", "")],
  ["gripping his collar", "a spinning takedown, legs whipping", "a crushing submission lock, body arched"],
  "tightening his belt with a smile",
  "slapping hands and bumping fists", 'grapple')

f('noah', 'he', 185, 'kyokushin karate fighter',
  "NOAH WILLIAMS, 25, Canadian kyokushin karate fighter, tall and heavily muscled. Pale skin, long dark bangs falling over his eyes, light stubble, a bruised cheek. A navy karate gi jacket with three-quarter sleeves worn open at the chest, navy gi trousers, a red cloth sash tied over a black belt, red knee wraps, barefoot.",
  "navy open gi jacket with three-quarter sleeves, navy trousers, black belt under a red sash, red knee wraps, long dark bangs, barefoot",
  "squared kyokushin stance, fists high in front of the chest, elbows wide",
  [("IRON CHEST", "squaring up, chest out", "a crushing straight punch to the chest, stepping in", "recovering"),
   ("ROUGH ROUNDHOUSE", "turning the hip over", "a heavy head-height roundhouse", "lowering the leg"),
   ("SEIKEN LOW", "dropping his stance", "a low driving punch toward the knee", "")],
  ["roaring with both fists down", "a heavy flurry of punches", "a towering axe kick"],
  "standing with fists pressed together at the chest, bowing",
  "punching his own chest twice", 'karate')

f('zara', 'she', 175, 'pehlwani wrestler',
  "ZARA KHAN, 28, Pakistani pehlwani wrestler, strong and broad-shouldered. Brown skin, dark hair braided and coiled at the back of the head, thick brows, fierce eyes. A plum sleeveless jersey top, loose ivory trousers tied at the ankles, a plum waist cloth, ivory knee wraps, barefoot.",
  "plum sleeveless top, loose ivory trousers tied at the ankles, plum waist cloth, ivory knee wraps, coiled dark braid, barefoot",
  "low wide wrestling stance, arms swinging forward, palms open",
  [("KUSTHI LIFT", "reaching low", "a powerful waist lift, back straight", "setting down"),
   ("GADA CRUSH", "swinging both arms up like a mace", "a crushing double-handed downward blow", "recovering"),
   ("CLINCH TOSS", "gripping forward", "a twisting hip toss", "")],
  ["slapping her shoulders", "a massive lift overhead", "slamming down on one knee"],
  "slapping her thighs and raising her arms",
  "rubbing dust on her palms", 'grapple')

f('thabo', 'he', 186, 'boxer',
  "THABO NKOSI, 27, South African boxer, tall, lean and long-armed. Very dark skin, short cropped hair with a shaved part line, a thin moustache, cool calm eyes. A black sleeveless boxing vest with sky-blue trim, sky-blue boxing trunks with a black waistband, white boxing gloves, black high boxing boots.",
  "black sleeveless vest with sky-blue trim, sky-blue trunks with black waistband, white gloves, black high boots, short cropped hair",
  "relaxed long-range boxing guard, lead hand low (philly shell), rear glove high by the chin, swaying",
  [("FLICKER JAB", "dropping the lead hand", "a long flicking jab at full reach", "retracting"),
   ("SHUFFLE HOOK", "shuffling forward", "a leaping uppercut-hook", "landing"),
   ("PIVOT CROSS", "pivoting off the line", "a crushing rear cross", "")],
  ["dancing the shuffle", "a dazzling combination", "a crushing uppercut"],
  "raising both white gloves to the sky",
  "bouncing on his toes, gloves low", 'boxing')

f('maia', 'she', 177, 'combat wrestler',
  "MAIA KORHONEN, 29, Finnish combat wrestler, tall and strong. Pale skin, ash-brown hair in a high ponytail, ice-blue eyes, a bandage on the bridge of her nose. A slate-grey fitted zip combat jacket with orange panels and rolled sleeves, slate shorts, orange knee pads, tall white wrestling boots.",
  "slate-grey zip jacket with orange panels, slate shorts, orange knee pads, tall white wrestling boots, ash-brown ponytail",
  "upright grappling guard, hands forward, ready to clinch",
  [("HIP TOSS", "stepping in and turning", "throwing with a full hip toss, back turned", "standing"),
   ("NORDIC TRAP", "catching high", "trapping and twisting with a rising arm lock", "releasing"),
   ("COLD COUNTER", "blocking with a forearm", "a short counter elbow", "")],
  ["rolling her shoulders", "lifting in a front waist lock", "a bridging suplex"],
  "pulling her ponytail tight with both hands",
  "stamping her boots and raising her hands", 'grapple')

f('tomas', 'he', 178, 'full-contact karate fighter',
  "TOMAS VEGA, 31, Spanish full-contact karate fighter, athletic and proud. Olive skin, swept-back brown hair, stubble, a confident smirk. A black karate gi with gold embroidered trim on the lapels and cuffs, a gold belt, black gi trousers, barefoot.",
  "black gi with gold trim on lapels and cuffs, gold belt, black trousers, swept brown hair, barefoot",
  "crouched karate stance, fists forward, weight low",
  [("TORO PUNCH", "chambering a fist at the hip", "a charging straight punch, stepping deep", "recovering"),
   ("FLAME AXE", "raising a straight leg high", "a crashing axe kick", "landing"),
   ("USHIRO GERI", "turning his back", "a spinning back kick straight out", "")],
  ["sweeping his gi aside", "a fast flurry of punches", "a leaping axe kick"],
  "running a hand through his hair and smirking",
  "flicking dust off his shoulder, then settling into stance", 'karate')

f('leila', 'she', 170, 'kickboxer',
  "LEILA BEN AMMAR, 24, Tunisian kickboxer, lean and fast. Warm tan skin, dark hair in a high ponytail, gold stud earrings, determined dark eyes. A cropped red-and-ivory track jacket open over a black sports top, red kickboxing shorts, black hand wraps, ivory shin pads, barefoot.",
  "cropped red-and-ivory track jacket over black top, red shorts, black hand wraps, ivory shin pads, dark high ponytail, barefoot",
  "high kickboxing guard, fists by the temples, bouncing",
  [("TIMED JAB", "stepping in", "a sharp counter jab at full reach", "retracting"),
   ("SAHARA KNEE", "grabbing forward", "a rising flying knee", "landing"),
   ("MEDITERRANEAN HOOK", "dipping the shoulder", "a looping hook", "")],
  ["bouncing on her toes", "a fast combination", "a leaping knee"],
  "blowing on her fist with a smirk",
  "tightening her wraps with her teeth", 'kickbox')

f('kaimana', 'he', 192, 'Samoan wrestler',
  "KAI MANA, 32, Samoan heavyweight wrestler, enormous and powerful with a big belly and huge arms. Brown skin, long black wavy hair tied back, a broad friendly face that turns fierce. Bold traditional tattoo bands across his thighs and hips. Bare chest. Teal patterned wrestling shorts, a red waist sash, black wristbands, barefoot.",
  "bare chest, teal patterned shorts with red sash, tattooed thighs, black wristbands, long black wavy hair tied back, barefoot",
  "wide heavyweight stance, leaning forward, arms wide",
  [("ISLAND TACKLE", "lowering his shoulder", "a charging body tackle", "slowing"),
   ("PACIFIC SLAP", "raising one huge hand", "a thunderous open-hand slap", "recovering"),
   ("SAMOAN DROP", "grabbing forward", "falling backward in a body drop", "")],
  ["a war chant with arms spread", "a leaping belly splash", "a crushing body drop"],
  "doing a war dance with tongue out and arms spread",
  "slapping his chest and thighs in a war dance", 'heavy')

f('aleksei', 'he', 182, 'sambo fighter',
  "ALEKSEI MOROZ, 30, Kazakh sambo fighter, lean and wiry, clean-shaven. Pale skin, blond hair cropped short with shaved sides, ice-blue eyes, a thin scar along the jaw. A cobalt-blue sambo jacket with slate panels and short sleeves, black belt, slate sambo shorts, tall black wrestling boots.",
  "cobalt-blue short-sleeved sambo jacket with slate panels, black belt, slate shorts, tall black boots, blond crop with shaved sides",
  "upright grappling stance, arms forward, hands open",
  [("STEPPING THROW", "stepping in", "a sweeping hip throw", "standing"),
   ("STEPPING ARMLOCK", "catching an arm", "a flying armlock", "landing"),
   ("STEP KICK", "stepping forward", "a low stepping side kick", "")],
  ["gripping forward", "a spinning throw", "a crushing knee drop"],
  "folding his arms with a cold stare",
  "pulling on his jacket collar", 'grapple')

f('hayato', 'he', 168, 'old karate and jujutsu master',
  "MASTER HAYATO, 68, Japanese old karate and jujutsu master, small, wiry and stooped but tough as iron. Long grey hair tied in a topknot, long white eyebrows, short grey beard, deep wrinkles, piercing eyes. A worn ivory gi top with brown wide hakama trousers, a brown sash, bare feet.",
  "worn ivory gi top, brown wide hakama trousers, brown sash, grey topknot, white eyebrows, short grey beard, barefoot",
  "relaxed open-palm stance, one palm forward, the other behind his back",
  [("ONE PUNCH KNOCKOUT", "drawing one fist back slowly", "a single devastating straight punch, robes snapping", "recovering"),
   ("OLD JUJUTSU PARRY", "raising an open palm", "catching and twisting with a wrist throw", "resetting"),
   ("ANCIENT SWEEP", "dropping low", "a low sweeping leg", "")],
  ["closing his eyes, palms together", "a blur of palm strikes", "a single crushing punch"],
  "stroking his beard and chuckling",
  "bowing slowly with hands on his knees", 'palm')

f('mason', 'he', 188, 'bare-knuckle brawler',
  "MAD DOG MASON, 38, American bare-knuckle brawler, big, heavy-muscled and rough. Messy red-brown hair, a broken nose with a scar across it, thick stubble, a missing tooth grin. A torn slate-grey sleeveless denim vest over a bare chest, a red bandana around his neck, black jeans, tan work boots, taped knuckles.",
  "torn slate-grey sleeveless denim vest over bare chest, red neck bandana, black jeans, tan work boots, taped knuckles, messy red-brown hair",
  "sloppy brawler guard, fists low and forward, swaying",
  [("HEADBUTT", "grabbing forward", "a vicious forward headbutt", "staggering back"),
   ("MAD RUSH", "lowering his head", "a charging swinging haymaker", "recovering"),
   ("DIRTY LOW", "dipping", "a low stomping kick", "")],
  ["cracking his knuckles", "a rain of wild haymakers", "a crushing overhand punch"],
  "spitting to the side and laughing",
  "cracking his neck and knuckles", 'boxing')

f('lotus', 'she', 168, 'silat assassin',
  "LOTUS, 30, Southeast Asian silat assassin, slim and deadly. Pale-golden skin, black hair in a high ponytail, cold narrow eyes, a dark scarf around her neck. A black sleeveless fitted wrap top with a dark-green sash, black split silat trousers, dark-green arm wraps, barefoot.",
  "black sleeveless wrap top, dark-green sash, black split trousers, dark-green arm wraps, high black ponytail, barefoot",
  "low silent stance, one knife-hand forward, the other high",
  [("SHADOW STEP", "fading low", "a darting knife-hand strike, body stretched", "fading back"),
   ("VIPER CUT", "spinning", "a rising spinning knife-hand slash", "landing"),
   ("DEFECTION PARRY", "weaving her hands", "a counter strike out of the deflection", "")],
  ["vanishing into a crouch", "a flurry of slashing hand strikes", "a final leaping strike"],
  "standing still with arms crossed, eyes closed",
  "dropping silently from above into stance", 'low')

f('tororojo', 'he', 200, 'heavyweight luchador',
  "EL TORO ROJO, 36, Mexican heavyweight luchador, massive and towering with a huge chest and thick arms. A red bull-shaped wrestling mask with short black horns and black trim. Bare massive torso. Black tights with red stripes, huge red lace-up boots, black wristbands, a black weightlifting belt.",
  "red bull mask with black horns, bare massive torso, black tights with red stripes, huge red boots, black wristbands",
  "wide heavyweight stance, arms open, head lowered like a bull",
  [("TORO PRESS", "lowering his horns", "a charging horned head ram", "slowing"),
   ("FLYING BULL", "leaping up", "a flying body press", "landing"),
   ("BULL BOMB", "grabbing forward", "a powerbomb, arms high", "")],
  ["pawing the ground like a bull", "lifting a huge weight overhead", "a crushing powerbomb"],
  "flexing both arms with a roar",
  "stamping and snorting, lowering his horns", 'heavy')

f('irina', 'she', 172, 'systema fighter',
  "IRINA VOLKOVA, 34, Russian systema specialist, slim and cool. Pale skin, a silver-white bob cut, cold grey eyes, thin lips. A slate-grey fitted high-collar tactical jacket, black cargo trousers, black tactical boots, black gloves.",
  "slate-grey high-collar tactical jacket, black cargo trousers, black boots, black gloves, silver-white bob",
  "relaxed loose stance, hands low and open, body soft",
  [("FLOWING PALM", "flowing forward", "a whipping palm strike", "resetting"),
   ("BIO-SPIRAL", "turning in a spiral", "a spiralling rising strike", "landing"),
   ("WAVE DEFLECTION", "swaying her shoulders", "a counter strike out of the deflection", "")],
  ["breathing calmly", "a flowing chain of strikes", "a final spiralling strike"],
  "brushing dust off her sleeve",
  "removing her gloves and sliding them back on", 'palm')

f('kofi', 'he', 180, 'traditional boxer',
  "KOFI MENSAH, 29, Ghanaian traditional boxer, muscular and compact. Very dark skin, short twists, a broad smile and confident eyes. Bare chest. Violet boxing shorts with an ivory patterned waistband, thick ivory cloth wraps on both fists, barefoot.",
  "bare chest, violet shorts with ivory patterned waistband, thick ivory fist wraps, short twists, barefoot",
  "compact boxing guard, fists tight, weaving",
  [("ACCRA HAMMER", "raising one fist high", "a crashing overhand hammer fist", "recovering"),
   ("IRON GUARD", "shelling up", "a rising uppercut out of the guard", "landing"),
   ("HEAVY HOOK", "dipping", "a crushing hook to the body", "")],
  ["bouncing", "a rain of hooks", "a leaping uppercut"],
  "flexing both arms with a big grin",
  "bumping his wrapped fists together", 'boxing')

f('xiaomei', 'she', 160, 'wing chun fighter',
  "XIAO MEI, 22, Chinese wing chun fighter, small and precise. Pale skin, long black hair in a single braid tied with red cord, sharp eyes. A black mandarin jacket with red piping and red frog buttons, black trousers, a red sash, black cloth slippers.",
  "black mandarin jacket with red piping and frog buttons, black trousers, red sash, black slippers, long black braid with red cord",
  "wing chun centreline guard, both hands forward on the centreline, one ahead of the other",
  [("CHAIN PUNCH", "stepping in", "a flurry of straight chain punches", "recovering"),
   ("PAK SAO PARRY", "slapping down a parry", "a rising palm counter", "resetting"),
   ("CENTER KICK", "lifting the knee", "a low straight front kick", "")],
  ["centring her hands", "a blur of chain punches", "a final palm strike"],
  "bowing with fist in palm",
  "performing a quick wing chun form", 'palm')

f('zero', 'he', 180, 'conditioned hybrid fighter',
  "ZERO, about 25, a lab-conditioned hybrid fighter, lean and pale with unnatural stillness. Long white fringe hanging over one eye, pale lilac eyes, dark circles. His right arm bandaged from shoulder to fingers in grey bandages with faint violet glowing seams beneath. A long slate-grey coat with violet lining worn open, a black fitted shirt, black trousers, black boots, a broken metal restraint collar around his neck.",
  "long open slate-grey coat with violet lining, black shirt and trousers, black boots, bandaged glowing right arm, long white fringe, broken metal collar",
  "unstable hunched stance, bandaged arm hanging forward, the other hand on his head",
  [("FRACTURED STRIKE", "drawing the bandaged arm back", "a lunging claw strike with the bandaged arm", "staggering"),
   ("MEMORY SURGE", "clutching his head", "a rising strike with the bandaged arm, violet light", "landing"),
   ("UNSTABLE WARP", "flickering", "a sudden teleporting kick", "")],
  ["screaming with the bandaged arm raised", "a frenzied strike", "a final crushing claw strike"],
  "laughing with his head tilted back",
  "staggering in as the bandages loosen", 'unstable')

f('echo', 'it', 175, 'combat android',
  "ECHO, an androgynous combat android built to imitate human fighters. A smooth pearl-white ceramic face mask with a single cyan optical slit, slate-grey segmented armour plates over a black bodysuit with teal glowing seams, sleek black boots.",
  "pearl-white ceramic mask with a cyan eye slit, slate segmented armour over a black bodysuit with teal glowing seams, black boots",
  "precise boxing guard, mechanical and symmetrical",
  [("MIMIC WAVE", "drawing a fist back", "a straight punch releasing a short wave of teal light", "retracting"),
   ("ECHO UPPER", "crouching", "a rising mechanical uppercut", "landing"),
   ("WARP DRIVE", "flickering", "a teleporting dash", "")],
  ["calibrating, arms out", "a rapid machine combination", "a final rising strike"],
  "standing perfectly still, scanning",
  "powering up with teal seams lighting", 'android')

f('atlas', 'it', 215, 'combat robot',
  "ATLAS, a massive combat robot, towering and heavy. A slate gunmetal armoured body, a narrow orange visor in a heavy helmet, huge hydraulic arms with pistons, thick armoured legs and heavy feet, orange warning stripes on the shoulders.",
  "slate gunmetal armour, narrow orange visor, huge hydraulic arms, orange warning stripes, heavy armoured legs",
  "heavy wide guard, huge fists raised",
  [("TITANIUM SHOCKWAVE", "raising both fists", "smashing the floor with both fists", "rising"),
   ("THRUSTER CHARGE", "leaning forward, thrusters firing", "a rocketing shoulder charge", "slowing"),
   ("HYDRAULIC PILEDRIVER", "grabbing forward", "a lifting slam with pistons extended", "")],
  ["venting steam", "a crushing hydraulic punch", "a ground-shaking slam"],
  "raising one massive fist, visor blinking",
  "unfolding from a crouch, visor lighting", 'heavy')

f('viper', 'she', 170, 'speed combat droid',
  "VIPER, a slender female combat droid built for speed. Short white synthetic hair panels, a smooth pale face with one green optical eye, a violet plated bodysuit with green glowing seams, slim armoured boots.",
  "violet plated bodysuit with green seams, short white hair panels, green optical eye, slim armoured boots",
  "light bouncing taekwondo-style guard, legs ready",
  [("VIPER FLASH", "crouching", "a blinding flying side kick", "landing"),
   ("CYBER SPIRAL", "pivoting", "a rising spinning kick", "landing"),
   ("PREDICTIVE EVADE", "leaning back", "a counter kick", "")],
  ["calibrating", "a flurry of kicks", "a final spinning kick"],
  "tilting her head, eye blinking",
  "activating, seams lighting green", 'taekwondo')

f('mirage', 'she', 176, 'engineered combat fighter',
  "MIRAGE, an adult female engineered fighter, the final boss: tall, graceful and cold. Pale skin, black asymmetric bob, lilac glowing eyes. A long black tailored combat coat with violet lining and silver buckles, black leggings, white high boots, black gloves.",
  "long black tailored coat with violet lining and silver buckles, black leggings, white high boots, black asymmetric bob, lilac eyes",
  "elegant open-hand stance, one palm forward, coat flowing",
  [("ADAPTIVE BLADE", "drawing her hand back", "a slicing knife-hand that leaves a violet light blade", "recovering"),
   ("NEURAL LEAP", "crouching", "a leaping spinning kick", "landing"),
   ("OPTICAL REFLECTION", "raising a palm", "a reflecting palm burst", "")],
  ["raising her hand, violet light gathering", "a storm of light-blade slashes", "a final violet burst"],
  "smiling coldly, coat settling",
  "materialising from violet light", 'palm')

FAMILY_NORMALS = {
    # (light punch, light kick, heavy punch, heavy kick, crouch light punch, crouch light kick,
    #  crouch heavy punch (anti-air), crouch heavy kick (sweep), jump light/heavy punch, jump light kick, jump heavy kick)
    'boxing': ("a quick lead jab", "a fast lead-hand body jab, dipping low", "a powerful straight right cross", "a looping overhand hook",
               "a crouching jab", "a low hook to the body, ducked", "a big rising uppercut", "a lunging low hook to the legs",
               "a diving downward punch in the air", "a mid-air knee", "a mid-air double-fist hammer blow"),
    'karate': ("a snapping front-hand jab", "a front snap kick at knee height", "a strong straight reverse punch", "a high roundhouse kick",
               "a crouching jab", "a low crouching snap kick", "a rising uppercut-punch", "a low spinning sweep",
               "a downward punch in the air", "a mid-air knee", "a flying side kick"),
}
FAMILY_NORMALS['default'] = ("a quick jab", "a low front kick", "a strong straight punch", "a high roundhouse kick",
                             "a crouching jab", "a low crouching kick", "a rising anti-air strike upward", "a low sweep",
                             "a downward punch in the air", "a mid-air knee", "a flying kick")
FAMILY_NORMALS['grapple'] = ("a quick jab", "a low front kick", "a stiff palm shove", "a heavy stomp kick",
                             "a crouching jab", "a low crouching kick", "a rising headbutt with arms up", "a low leg sweep",
                             "a downward chop in the air", "a mid-air knee", "a mid-air drop kick with both feet")
FAMILY_NORMALS['heavy'] = ("a heavy jab", "a short stomping kick", "a huge swinging palm slap", "a heavy forward kick",
                           "a crouching jab", "a low stomp", "a rising double-arm strike upward", "a low sweeping shove",
                           "a downward body splash in the air", "a mid-air knee", "a mid-air body press")
FAMILY_NORMALS['thai'] = ("a quick jab", "a teep push kick", "a slashing elbow", "a high roundhouse kick with the shin",
                          "a crouching jab", "a low kick to the calf", "a rising uppercut elbow", "a low sweeping shin kick",
                          "a downward elbow in the air", "a flying knee", "a mid-air roundhouse"),
FAMILY_NORMALS['thai'] = FAMILY_NORMALS['thai'][0]
FAMILY_NORMALS['palm'] = ("a quick palm jab", "a low snapping kick", "a strong double palm strike", "a high crescent kick",
                          "a crouching palm strike", "a low crouching kick", "a rising palm upward", "a low spinning sweep",
                          "a downward palm in the air", "a mid-air knee", "a flying kick")
for fam in ('kickbox', 'savate', 'taekwondo', 'flow', 'low', 'elbow', 'lucha', 'unstable', 'android'):
    FAMILY_NORMALS.setdefault(fam, FAMILY_NORMALS['default'])

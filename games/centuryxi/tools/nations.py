"""The 48 national teams: one source for the game's tables and the flags.

Kits are plain team colours (no crests, no maker's marks). Players are original
characters: every name here is generated, and none stands for a real footballer.
Attributes run 1-16. Formations: 0 4-4-2, 1 4-3-3, 2 3-5-2, 3 5-3-2.
"""
import random
from pathlib import Path

W, K = 0xF4F4F8, 0x16161C        # white, near black

#  name, code, region, formation, (speed, power, press, control, defence, keeper),
#  home (shirt, shorts, socks), away (shirt, shorts, socks), keeper shirt
NATIONS = [
    ("BRAZIL", "BRA", "br", 0, (15, 12, 12, 16, 11, 12), (0xF8D010, 0x1848B0, W), (0x1848B0, W, 0x1848B0), 0x30A050),
    ("ARGENTINA", "ARG", "es", 0, (13, 12, 14, 16, 13, 13), (0x78B8EC, K, W), (0x202C70, 0x202C70, 0x202C70), 0xE8C820),
    ("URUGUAY", "URU", "es", 0, (12, 14, 15, 12, 14, 12), (0x60BCEC, K, K), (W, K, W), 0xE08020),
    ("COLOMBIA", "COL", "es", 1, (13, 11, 12, 13, 11, 11), (0xF8D010, 0x1848B0, 0xD02030), (0xD02030, 0xD02030, 0xD02030), 0x30A050),
    ("MEXICO", "MEX", "es", 0, (13, 10, 12, 12, 10, 12), (0x0C7048, W, 0xD02030), (W, 0x0C7048, W), 0xE8C820),
    ("UNITED STATES", "USA", "en", 0, (13, 12, 13, 10, 10, 11), (W, 0x0C2C68, W), (0x0C2C68, 0xD02030, 0x0C2C68), 0xE08020),
    ("CANADA", "CAN", "en", 0, (14, 12, 12, 10, 9, 10), (0xD81020, 0xD81020, 0xD81020), (W, W, W), 0x202C70),
    ("COSTA RICA", "CRC", "es", 3, (11, 9, 11, 10, 11, 13), (0xD02030, 0x102C80, W), (W, W, W), 0xE8C820),
    ("FRANCE", "FRA", "fr", 1, (16, 13, 12, 15, 13, 13), (0x102C70, W, 0xD82828), (W, 0x102C70, W), 0x30A050),
    ("GERMANY", "GER", "de", 2, (12, 15, 14, 14, 14, 15), (W, K, W), (0x18683C, W, 0x18683C), 0xE8C820),
    ("ITALY", "ITA", "it", 0, (11, 13, 12, 14, 16, 15), (0x1068C8, W, 0x1068C8), (W, 0x1068C8, W), 0x30A050),
    ("SPAIN", "ESP", "es", 1, (12, 10, 14, 16, 13, 13), (0xC81020, 0x102C70, K), (0xF0C820, 0xC81020, 0xF0C820), 0x30A050),
    ("ENGLAND", "ENG", "en", 0, (13, 14, 13, 13, 13, 12), (W, 0x102060, W), (0xD02030, W, 0xD02030), 0xE8C820),
    ("PORTUGAL", "POR", "br", 1, (13, 12, 11, 15, 12, 13), (0xA81828, 0x0C7040, 0xA81828), (W, W, W), 0xE8C820),
    ("NETHERLANDS", "NED", "nl", 1, (13, 13, 13, 14, 12, 12), (0xF07020, W, 0xF07020), (0x102060, 0x102060, 0x102060), 0x30A050),
    ("BELGIUM", "BEL", "nl", 1, (13, 13, 12, 14, 12, 14), (0xC81030, K, 0xC81030), (W, W, W), 0xE8C820),
    ("CROATIA", "CRO", "hr", 0, (11, 12, 13, 14, 12, 12), (W, 0xD81828, 0x1848A0), (0x102060, 0x102060, 0x102060), 0xE8C820),
    ("SWEDEN", "SWE", "no", 0, (11, 14, 12, 11, 12, 12), (0xF8CC00, 0x0868A8, 0xF8CC00), (0x0868A8, 0x0868A8, 0x0868A8), 0x30A050),
    ("ROMANIA", "ROU", "ro", 0, (11, 11, 11, 13, 11, 11), (0xF8D010, 0x102C80, 0xF8D010), (0xD02030, 0xD02030, 0xD02030), 0x30A050),
    ("NORWAY", "NOR", "no", 0, (13, 15, 12, 11, 10, 11), (0xB81030, W, 0x102060), (W, 0x102060, W), 0xE8C820),
    ("MOROCCO", "MAR", "ar", 1, (14, 11, 14, 13, 14, 13), (0xC02830, 0x086830, 0xC02830), (W, W, W), 0xE8C820),
    ("EGYPT", "EGY", "ar", 0, (13, 11, 11, 13, 10, 11), (0xD02030, W, K), (W, K, W), 0x30A050),
    ("NIGERIA", "NGA", "ng", 0, (15, 14, 11, 12, 10, 10), (0x0C8850, W, 0x0C8850), (W, 0x0C8850, W), 0xE8C820),
    ("SENEGAL", "SEN", "sn", 0, (15, 14, 13, 12, 12, 12), (W, W, 0x0C8848), (0x0C8848, 0x0C8848, 0xF0D020), 0xD02030),
    ("CAMEROON", "CMR", "sn", 0, (14, 14, 12, 11, 11, 11), (0x087C60, 0xD02030, 0xF8D010), (0xD02030, 0xD02030, 0xD02030), 0x202C70),
    ("GHANA", "GHA", "gh", 0, (14, 13, 12, 12, 10, 10), (W, W, W), (0xD02030, 0xD02030, 0xF0D020), 0x30A050),
    ("JAPAN", "JPN", "jp", 1, (14, 9, 15, 14, 11, 11), (0x102C8C, W, 0x102C8C), (W, 0x102C8C, W), 0xE8C820),
    ("SOUTH KOREA", "KOR", "kr", 0, (14, 10, 15, 12, 11, 11), (0xD03040, K, 0xD03040), (W, W, W), 0x30A050),
    ("AUSTRALIA", "AUS", "en", 0, (12, 13, 13, 10, 10, 11), (0xF8D000, 0x08843C, 0xF8D000), (0x08843C, 0xF8D000, 0x08843C), 0x202C70),
    ("SAUDI ARABIA", "KSA", "ar", 0, (12, 9, 11, 11, 9, 10), (W, W, W), (0x086C38, 0x086C38, 0x086C38), 0xE8C820),
    ("IRAN", "IRN", "ir", 3, (12, 12, 12, 11, 12, 13), (W, W, W), (0xD02030, 0xD02030, 0xD02030), 0x30A050),
    ("QATAR", "QAT", "ar", 2, (11, 9, 10, 11, 9, 10), (0x8C1838, W, 0x8C1838), (W, 0x8C1838, W), 0xE8C820),
    # the great football nations the first 32 left out (World Cup winners' rivals, a 1994 semi-finalist,
    # a European champion), and the most populous nation of all: 48, a 2026 finals' worth
    ("CHINA", "CHN", "cn", 0, (12, 10, 12, 11, 11, 11), (0xD81E2A, 0xD81E2A, 0xD81E2A), (W, W, W), 0x30A050),
    ("DENMARK", "DEN", "dk", 0, (13, 13, 13, 13, 13, 13), (0xC8102E, W, 0xC8102E), (W, 0xC8102E, W), 0x30A050),
    ("SWITZERLAND", "SUI", "ch", 0, (12, 12, 13, 13, 14, 13), (0xD52B1E, W, 0xD52B1E), (W, 0xD52B1E, W), 0xE8C820),
    ("POLAND", "POL", "pl", 0, (12, 13, 12, 12, 12, 13), (W, 0xDC143C, W), (0xDC143C, W, 0xDC143C), 0x30A050),
    ("SERBIA", "SRB", "rs", 0, (12, 13, 12, 13, 12, 12), (0xC6363C, 0x0C4076, W), (W, W, W), 0xE8C820),
    ("TURKEY", "TUR", "tr", 1, (12, 12, 13, 12, 12, 12), (0xE30A17, W, 0xE30A17), (W, 0xE30A17, W), 0x202C70),
    ("SCOTLAND", "SCO", "sc", 0, (11, 12, 14, 11, 12, 11), (0x1C2C5C, W, 0x1C2C5C), (W, 0x1C2C5C, W), 0xE8C820),
    ("CZECHIA", "CZE", "cz", 0, (12, 12, 12, 13, 12, 13), (0xD7141A, W, 0x11457E), (W, W, W), 0x30A050),
    ("BULGARIA", "BUL", "bg", 0, (12, 12, 12, 13, 11, 12), (W, 0x00966E, 0xD62612), (0xD62612, 0xD62612, 0xD62612), 0xE8C820),
    ("GREECE", "GRE", "gr", 3, (11, 12, 13, 11, 14, 13), (W, W, W), (0x0D5EAF, 0x0D5EAF, 0x0D5EAF), 0xE8C820),
    ("CHILE", "CHI", "an", 1, (13, 12, 14, 12, 12, 12), (0xD52B1E, 0x0039A6, W), (W, W, W), 0x202C70),
    ("PERU", "PER", "an", 0, (12, 10, 12, 13, 11, 11), (W, W, W), (0xD91023, 0xD91023, 0xD91023), 0x30A050),
    ("PARAGUAY", "PAR", "an", 0, (11, 12, 13, 11, 13, 12), (0xD52B1E, 0x0038A8, 0x0038A8), (W, W, W), 0x30A050),
    ("ECUADOR", "ECU", "an", 0, (14, 12, 13, 11, 12, 11), (0xFFD100, 0x034EA2, 0x034EA2), (0x034EA2, W, 0x034EA2), 0x30A050),
    ("IVORY COAST", "CIV", "ci", 0, (15, 14, 12, 12, 11, 11), (0xF77F00, W, 0x009E60), (W, 0x009E60, W), 0x202C70),
    ("ALGERIA", "ALG", "dz", 3, (13, 12, 12, 13, 12, 12), (W, 0x006233, W), (0x006233, W, 0x006233), 0xE8C820),
]

# What the game shows: each side is a parody nation, recognisable by its colours,
# its flag's layout and a cute symbol (tools/flags.py), never by its real name. Each
# name plays on something the country is proud of (a dish, a dance, a landscape, a
# motto): friendly, never a jibe a supporter of that country would mind.
# The three-letter keys above stay the tables' internal keys.
PARODY = {
    "BRA": ("SAMBALIA", "SMB"), "ARG": ("ARGENTANGO", "ATG"), "URU": ("MATEGUAY", "MTG"), "COL": ("COFFEELOMBIA", "CFE"),
    "MEX": ("MARIACHIA", "MRC"), "USA": ("BURGERLAND", "BGL"), "CAN": ("MAPLEDONIA", "MPD"), "CRC": ("PURAVIDIA", "PVD"),
    "FRA": ("BAGUETTIA", "BGT"), "GER": ("WURSTLAND", "WST"), "ITA": ("PASTALIA", "PST"), "ESP": ("PAELLANIA", "PAE"),
    "ENG": ("TEALAND", "TEA"), "POR": ("BACALANDIA", "BCL"), "NED": ("TULIPLANDS", "TLP"), "BEL": ("WAFFLEGIUM", "WFG"),
    "CRO": ("CRAVATIA", "CVT"), "SWE": ("MEATBALLIA", "MBL"), "ROU": ("CARPATHIA", "CRP"), "NOR": ("FJORDWAY", "FJW"),
    "MAR": ("MINTOROCCO", "MNT"), "EGY": ("PYRAMIDIA", "PYR"), "NGA": ("JOLLOFIA", "JLF"), "SEN": ("SENEGOAL", "SNG"),
    "CMR": ("CAMEROAR", "RAR"), "GHA": ("CHOCOGHANA", "CHG"), "JPN": ("SUSHIPON", "SSP"), "KOR": ("KIMCHIREA", "KMC"),
    "AUS": ("KANGAROOLIA", "KNG"), "KSA": ("PALMARABIA", "PLM"), "IRN": ("PERSICATIA", "PRS"), "QAT": ("PEARLATAR", "PRL"),
    # silk (the road it named), hygge, fondue, pierogi, kajmak, Turkish delight, tartan, crystal,
    # the rose valley, feta, the condor, the llama, the harp, the line round the world, the elephants
    # and the fennec fox the sides are themselves called after
    "CHN": ("SILKINA", "SLK"), "DEN": ("HYGGEMARK", "HYG"), "SUI": ("FONDUZERLAND", "FDZ"), "POL": ("PIEROLAND", "PRG"),
    "SRB": ("KAJMAKIA", "KJM"), "TUR": ("DELIGHTIYE", "DLT"), "SCO": ("TARTANLAND", "TRT"), "CZE": ("CRYSTALIA", "CYS"),
    "BUL": ("ROSEGARIA", "RSG"), "GRE": ("FETALLAS", "FTL"), "CHI": ("CONDORIA", "CND"), "PER": ("LLAMARU", "LMA"),
    "PAR": ("HARPAGUAY", "HRP"), "ECU": ("EQUATORIA", "EQT"), "CIV": ("ELEPHANTOIRE", "ELP"), "ALG": ("FENNECIA", "FNC"),
}


def shown(n):
    """A side's name and code as the game shows them."""
    return PARODY[n[1]]


# original shirt names by region: cute joke names (food, animals, things), none a real person
NAMES = {
    "br": "BANANINHO COCADINHO PIPOQUINHA CAFEZINHO SAMBINHA TAPIOCA PACOQUINHA BRIGADEIRO GOIABINHA FAROFINHA PASTELAO CAIPIRINHO FEIJAOZIN MANDIOQUIN ACAIZINHO PAOZINHO QUEIJINHO BOLINHO PUDIMZINHO MILHOZINHO PICOLE GUARANITO CUSCUZINHO TUCANINHO".split(),
    "es": "CHURRITO NACHITO GUACAMOLO PAPITAS EMPANADA CHORIPAN TAMALITO QUESADILLO BURRITO DULCITO CHIMICHURI ALFAJOR MANZANITA PIMIENTO TORTILLO GAZPACHO MATECITO POLLITO PATATITA CEVICHITO ARROCITO CHURRASCO PIMENTON LIMONCITO".split(),
    "fr": "BAGUETTO FROMAGIN CREPELOT ESCARGOT CAMEMBERT CROQUETTE BONBON MADELEINE CHOUQUETTE RATATOUIL TARTIFLET BRIOCHET SOUFFLE CANARD GATEAU CHOUFLEUR OMELETTE BEIGNET CASSOULET MOUTARDE PATISSOU MACARON CROISSANT PETITPOIS".split(),
    "de": "BREZELMANN WURSTEL KARTOFFEL STRUDELL SAUERKRAUT KNODEL SCHNITZEL BRATWURST LEBKUCHEN PUMPERNIK SPATZLE KUCHENBERG SENFMANN GURKENHOF KOHLRABI MAULTASCH BRATAPFEL KRAPFEN ZWIEBACK BROTCHEN APFELSTRU KASESPATZ DACKELMANN PUDELHOF".split(),
    "it": "SPAGHETTI RAVIOLI LASAGNO TORTELLINO PEPPERONI GELATINO MOZZARELO BRUSCHETTO RISOTTO CANNOLINO PANETTONE MINESTRONE CARBONARA POMODORO FOCACCIO GNOCCHETTO TIRAMISU BISCOTTO PESTONE RICOTTINO FUSILLI LINGUINI CAPRESE PIZZETTA".split(),
    "en": "CRUMPET MUFFINTON SCONEY BISCUITS PUDDINGTON TEABAGS CHIPPY BANGERS PICKLES JAMMY TOFFEE CUSTARD TRIFLE KIPPER TEACAKE PORRIDGE BEANS GRAVY HOTPOT CRUMBLE CHUTNEY PASTY PANCAKES WAFFLES BAGELS HOTDOG LAMINGTON PAVLOVA KOALA WOMBAT".split(),
    "nl": "STROOPWAFL HAGELSLAG BITTERBAL POFFERTJE FRIETJES DROPJES SPECULAAS TULPJES KLOMPEN GOUDA EDAMMER PANNENKOEK OLIEBOL WAFELTJE MOSSELMAN PRALINEKE CHOCOLAAT SPRUITJE STAMPPOT KROKETJE VLAAI HUTSPOT MOLENAAR FIETSJES".split(),
    "hr": "CEVAPCIC PALACINKA SARMICA KRAVATIC BUREKOVIC SIRNIC AJVARIC KOLACIC FRITULIC STRUKLIC PAPRIKIC TARTUFIC MASLINIC SOKOLIC JADRANIC GALEBIC DALMATIC RIBICIC TIKVIC OREHIC MEDOVIC".split(),
    "no": "KOTTBULLE LINGONSEN KANELBULLE SALMONSEN FJORDSEN TROLLSTAD VIKINGSEN MOOSEBERG GJETOST KRUMKAKE HERRINGSON ELKSTROM SNOWBERG SKIBERG SAUNASSON ICEBERG AURORASEN RENSDYR WAFFLESEN TROLLHAUG LEFSE PUFFINSEN".split(),
    "ro": "CARPATESCU BRADESCU MUNTESCU COZONACU SARMALESCU MICILESCU MAMALIGA PAPANASI CIORBESCU BRANZESCU COVRIGEL PLACINTESC GOGOSESCU LUPESCU URSULESCU DANUBESCU CASTELESCU STELESCU CAPELESCU MUSTACESCU POIENESCU".split(),
    "ar": "FALAFELI HUMMUSI COUSCOUSI SHAWARMI BAKLAWI TAJINI DATESI KUNAFI FATTOUSHI KEBABI SAMBUSI LABNEHI MAAMOULI HALWANI CAMELLO FALCONI SANDDUNI OASISI MIRAGI PYRAMIDI SPHINXI PAPYRUSI KOSHARI MINTEAI".split(),
    "ng": "JOLLOFOR PUFFPUFF SUYAKUNLE CHINCHIN EGUSIOBI PLANTAINO AKARAEZE MOINMOIN ZOBO KILISHI OKRAFOR POUNDEDYAM OGBONOBI FUFUOLA GARRIBA ASUNOBI EFOEKEKE".split(),
    "sn": "THIEBOUDIE YASSALAYE MAFEDIOP BISSAPFALL BAOBABA TERANGA LIONCEAU NDOLEKANE BOBOLO MBONGO KOKI ACCRAS PASTELLE DIBITERIE CAFETOUBA CHAKRI NIEBE FATAYA SOMBI".split(),
    "gh": "JOLLOFAH KENKEY BANKU FUFUAH WAAKYE KELEWELE COCOAH KOKOBEH AMPESI PLANTAH GROUNDNUT AGUSHI TATALE KOOSE AKPLE REDRED BOFROT ANKWA".split(),
    "jp": "ONIGIRI MOCHIDA TEMPURA WASABI RAMENO SUSHIDA DANGO UDONO TAKOYAKI SOBAYASHI NATTO MISOSHIRU GYOZA EDAMAME TERIYAKI TOFUKAWA NORIMAKI UMEBOSHI KATSUDON YAKITORI MATCHA SENBEI DORAYAKI OKONOMI TAIYAKI".split(),
    "kr": "KIMCHI BIBIMBAP BULGOGI TTEOKBOKI MANDU JAPCHAE GIMBAP HOTTEOK SAMGYUP DUBU BINGSU GOCHU SUNDUBU PAJEON GALBI BANCHAN SOONDAE KONGNAMUL DOSIRAK YAKGWA SONGPYEON BUNGEO".split(),
    "ir": "KEBABIAN SAFFRONI PISTACHI TAHDIGI FESENJAN GHORMEH ASHRESHTE BARBARI SANGAKI DOOGHI FALOODEH SHOLEH KASHKI MEOWZADEH PURRSIAN WHISKERI KITTENI FLUFFZADEH TABBYAN CATIZADEH".split(),
    "cn": "BAOZI JIAOZI DIMSUM WONTONG MANTOU TANGYUAN HOTPOTTO MOONCAKE BOKCHOI KUNGPAO CHAOMIAN XIAOLONG ZONGZI YOUTIAO PEKINGO LANTERNO SILKWORM JADEMAN BAMBOO PANDABAO DUMPLINGO SESAMI".split(),
    "dk": "SMORREBROD FRIKADEL WIENERBROD AEBLESKIVE RUGBROD HYGGELIG PASTRYSEN HERRINGSEN REMOULADE KRINGLE FLODEBOLLE BRUNSVIGER KANELSNEGL MARZIPAN ROLLEPOLSE PANDEKAGE VIKINGBO CANDLESEN".split(),
    "ch": "FONDUELI RACLETTI ROSTIMANN MUESLIGER CHOCOLATLI CUCKOOLI ALPHORNLI EMMENTAL GRUYERELI BIRCHERLI ZOPFLI LECKERLI BERNERLI MATTERLI WATCHLI EDELWEISS STEINBOCK YODELLI".split(),
    "pl": "PIEROGSKI KIELBASKI BIGOSKI PACZKOWSKI BARSZCZYK ZUREKOWSKI GOLABKI OSCYPEK SERNIKSKI MAKOWIEC OBWARZANEK KOPYTKA PLACKI KREMOWKA SLEDZIK BOCIANSKI POLONEZ AMBEROWSKI".split(),
    "rs": "KAJMAKOVIC PLJESKAVIC AJVAROVIC GIBANICIC SARMIC PROJARIC KACAMAKIC PITICA BUREKIC SLATKOVIC URNEBESIC KOMPOTIC PASULJIC VANILICIC KIFLICIC TULUMBIC LEPINJIC SIRCETIC".split(),
    "tr": "BAKLAVACI KEBABCI SIMITCI LOKUMCU DONERCI BOREKCI AYRANCI MANTICI KUNEFECI PIDECI LAHMACUN DOLMACI CAYCI KOFTECI SUCUKCU HELVACI TULUMBACI GOZLEMECI".split(),
    "sc": "HAGGISON SHORTBREAD TATTIES NEEPS CRANACHAN BANNOCK KIPPERSON PORRIDGER TABLET CULLENSKIN TARTANSON BAGPIPER NESSIEMAN THISTLE HIGHLANDER LOCHSIDE CEILIDH SPORRAN".split(),
    "cz": "KNEDLIK TRDELNIK KOLACEK SVICKOVA BUCHTA LIVANECEK BRAMBORAK UTOPENEC KARLOVAR SKLENICKA LOUTKAR VANOCKA MEDOVNIK HOUSKA ROHLICEK ZELNACEK BABOVKA KRISTALEK".split(),
    "bg": "BANITSAROV SHOPSKOV KISELOV LYUTENICOV KOZUNAKOV TARATOROV SIRENOV KEBAPCHEV KAVARMOV GYUVECHEV ROSEMILKOV MEKITSOV BOZOV PALACHINOV TUTMANIKOV ROSEVALOV".split(),
    "gr": "SOUVLAKIS GYROPOULOS FETAKIS MOUSSAKAS BAKLAVAS TZATZIKIS DOLMADES SPANAKOS LOUKOUMAS OLIVOPOLIS FRAPPEDIS GALAKTOS KOULOURIS HALLOUMIS PASTITSIOS TARAMAS KALAMARIS ORZOPOULOS".split(),
    "an": "CEVICHON QUINOITO LLAMITO ALPAQUITO CONDORITO CHOCLITO HUMITA EMPANADON SOPAIPILLA ANTICUCHO CAUSITA LOMITO CHIRIMOYO LUCUMITO PEBRE CAZUELA MOTECITO CHIPA SOPITA MANDIOCO MAICITO PANCHITO TERERITO PICARON".split(),
    "ci": "ATTIEKE ALLOCO GARBA KEDJENOU FOUTOU PLACALI ABOLO GNAMAKOU CLAKO DEGUE AKPANI BOFLOTO BISSAPOU CACAOUTE ELEPHANTO LAGUNE IVOIRO MAQUISARD".split(),
    "dz": "CHORBAI MAKROUDI ZLABIA MHAJEB BOUREKI GARANTITA CHAKCHOUK BAGHRIR MSEMEN KESRA SFENJ MAQROUT FENNECI DATTOUI SAHARI KASBAHI ATLASSI ORANI".split(),
}
AVOID = {"SON", "MESSI", "NEYMAR", "SALAH", "KUBO", "MBAPPE", "HAKIMI", "HAALAND", "RONALDO", "BAGGIO", "MALDINI", "HAGI", "OKOCHA", "BERGKAMP", "KLINSMANN"}

# figure 0 spiky, 1 wild frizz and goatee, 2 mullet and stubble, 3 shaved, 4 dreadlocks and moustache;
# skin 0 light, 1 fair-tan, 2 tan, 3 brown, 4 dark, 5 warm light; hair ramps below
LOOKS = {
    "br": ((0, 1, 2, 3, 4), (0, 1, 2)), "es": ((0, 1, 2, 2, 3), (0, 1, 2)), "fr": ((0, 1, 3, 4, 4), (0, 1, 2)),
    "de": ((0, 0, 0, 1, 3), (0, 2, 2)), "it": ((0, 1, 1, 2, 2), (0, 2, 2)), "en": ((0, 0, 1, 3, 4), (0, 1, 2)),
    "nl": ((0, 0, 1, 3, 4), (0, 2, 2)), "hr": ((0, 0, 1, 1, 1), (0, 2, 2)), "no": ((0, 0, 0, 0, 1), (2, 2, 0)),
    "ro": ((0, 1, 1, 2, 2), (0, 2, 2)), "ar": ((1, 2, 2, 3, 3), (0, 0, 1)), "ng": ((3, 4, 4, 4, 4), (0, 1, 1)),
    "sn": ((3, 4, 4, 4, 4), (0, 1, 1)), "gh": ((3, 4, 4, 4, 4), (0, 1, 1)), "jp": ((0, 0, 1, 1, 1), (0, 0, 2)),
    "kr": ((0, 0, 1, 1, 1), (0, 0, 2)), "ir": ((1, 1, 2, 2, 3), (0, 0, 2)),
    "cn": ((5, 5, 5, 0, 1), (0, 0, 2)), "dk": ((0, 0, 0, 1, 3), (0, 2, 2)), "ch": ((0, 0, 1, 1, 3), (0, 2, 2)),
    "pl": ((0, 0, 0, 0, 1), (0, 2, 2)), "rs": ((0, 0, 1, 1, 1), (0, 2, 2)), "tr": ((1, 1, 2, 2, 2), (0, 0, 2)),
    "sc": ((0, 0, 0, 0, 1), (0, 2, 2)), "cz": ((0, 0, 0, 1, 1), (0, 2, 2)), "bg": ((0, 1, 1, 2, 2), (0, 2, 2)),
    "gr": ((1, 1, 2, 2, 2), (0, 0, 2)), "an": ((1, 2, 2, 3, 3), (0, 0, 1)), "ci": ((3, 4, 4, 4, 4), (0, 1, 1)),
    "dz": ((1, 2, 2, 3, 3), (0, 0, 1)),
}
HAIR_BY_REGION = {"no": (5, 4, 3, 2, 1), "de": (1, 2, 4, 3, 5), "nl": (1, 2, 4, 3, 5), "en": (1, 2, 3, 4, 5), "hr": (0, 1, 2, 2, 3),
                  "ro": (0, 1, 1, 2, 2), "ng": (0, 0, 0, 0, 1), "sn": (0, 0, 0, 0, 1), "gh": (0, 0, 0, 0, 1), "jp": (0, 0, 0, 0, 1),
                  "kr": (0, 0, 0, 0, 1), "ar": (0, 0, 1, 1, 0), "ir": (0, 0, 1, 1, 0), "br": (0, 1, 1, 2, 4), "es": (0, 1, 1, 2, 2),
                  "fr": (0, 1, 1, 2, 3), "it": (0, 1, 1, 2, 2),
                  "cn": (0, 0, 0, 0, 1), "dk": (4, 5, 2, 1, 3), "ch": (1, 2, 2, 4, 1), "pl": (1, 2, 4, 5, 2), "rs": (0, 1, 1, 2, 0),
                  "tr": (0, 0, 1, 1, 0), "sc": (3, 3, 2, 4, 1), "cz": (1, 2, 2, 4, 1), "bg": (0, 1, 1, 2, 0), "gr": (0, 0, 1, 1, 0),
                  "an": (0, 0, 0, 1, 1), "ci": (0, 0, 0, 0, 1), "dz": (0, 0, 1, 1, 0)}
# Which of the five figures a region's men are drawn as. The East Asian sides are mostly
# the short spiky cut of their 1990s sides, some shaved, the odd longer cut; nobody there
# gets the frizz and goatee or the locks. Elsewhere any cut, locks only where skins run dark.
FIG_BY_REGION = {"jp": (0, 0, 0, 0, 0, 3, 2), "kr": (0, 0, 0, 0, 0, 3, 3), "cn": (0, 0, 0, 0, 0, 3, 2)}

# Player types: how a man plays. 0 plain 1 speed 2 dribbler 3 target man 4 playmaker 5 shooter 6 stopper 7 set pieces
T_PLAIN, T_SPEED, T_DRIBBLE, T_TARGET, T_PLAYMAKER, T_SHOOTER, T_STOPPER, T_SETPIECE = range(8)

# A national side's most recognisable TYPE of 1990s player: how he looks and how he plays, never a
# person. (code, squad slot) -> figure, skin, hair, type
STARS = {("COL", 8): (1, 2, 4, T_PLAYMAKER), ("BRA", 9): (3, 3, 0, T_SHOOTER), ("BRA", 10): (1, 3, 0, T_DRIBBLE), ("BRA", 2): (2, 3, 0, T_SETPIECE),
         ("GER", 9): (2, 0, 4, T_TARGET), ("GER", 2): (2, 0, 1, T_STOPPER), ("ITA", 8): (2, 1, 1, T_SETPIECE), ("ITA", 2): (2, 1, 0, T_STOPPER),
         ("ARG", 9): (1, 1, 1, T_SHOOTER), ("ARG", 8): (2, 1, 0, T_DRIBBLE), ("NED", 9): (4, 3, 0, T_TARGET), ("NED", 8): (0, 0, 0, T_PLAYMAKER),
         ("ROU", 8): (2, 1, 0, T_PLAYMAKER), ("NGA", 9): (4, 4, 0, T_SPEED), ("NGA", 8): (1, 4, 0, T_DRIBBLE), ("CMR", 10): (3, 4, 0, T_SHOOTER),
         ("ENG", 8): (2, 0, 3, T_PLAYMAKER), ("ENG", 9): (3, 0, 0, T_TARGET), ("FRA", 9): (3, 1, 0, T_DRIBBLE), ("SWE", 9): (2, 0, 5, T_TARGET),
         ("URU", 9): (1, 1, 0, T_SHOOTER), ("POR", 9): (0, 1, 0, T_DRIBBLE), ("ESP", 8): (0, 1, 0, T_PLAYMAKER), ("MEX", 9): (0, 2, 0, T_SETPIECE),
         ("JPN", 8): (0, 0, 0, T_PLAYMAKER), ("KOR", 9): (0, 0, 0, T_SPEED), ("SEN", 9): (4, 4, 0, T_SPEED), ("EGY", 9): (1, 2, 0, T_SPEED)}

# Each side plays its own way: 0 passing, 1 direct, 2 flair, 3 counter. From how the sides
# played at the 1994 finals (Brazil's 4-4-2, Italy's counter, Germany's sweeper and 3-5-2,
# Romania's counter round a playmaker) and how some play today (France on the break).
STYLE = {"BRA": 2, "ARG": 2, "URU": 3, "COL": 0, "MEX": 0, "USA": 1, "CAN": 1, "CRC": 3, "FRA": 3, "GER": 1, "ITA": 3, "ESP": 0, "ENG": 1, "POR": 2,
         "NED": 0, "BEL": 0, "CRO": 0, "SWE": 1, "ROU": 3, "NOR": 1, "MAR": 3, "EGY": 3, "NGA": 2, "SEN": 2, "CMR": 2, "GHA": 2, "JPN": 0, "KOR": 3,
         "AUS": 1, "KSA": 3, "IRN": 3, "QAT": 3,
         "CHN": 3, "DEN": 0, "SUI": 3, "POL": 1, "SRB": 1, "TUR": 2, "SCO": 1, "CZE": 0, "BUL": 2, "GRE": 3, "CHI": 2, "PER": 0,
         "PAR": 3, "ECU": 1, "CIV": 2, "ALG": 3}

# How each side is known for celebrating. The numbers are CX_C_ in scenes/cx_sim.h.
C_RUN, C_KNEE, C_SHIRT, C_BABY, C_DANCE, C_HIPS, C_BELLY, C_FLIP, C_SAMBA, C_PRAY, C_SCREAM, C_SKY, C_SKATE, C_JUMP = range(14)
CELEB = {"BRA": C_BABY, "ARG": C_SCREAM, "URU": C_SKY, "COL": C_SAMBA, "MEX": C_FLIP, "USA": C_RUN, "CAN": C_RUN, "CRC": C_DANCE, "FRA": C_KNEE, "GER": C_BELLY,
         "ITA": C_SCREAM, "ESP": C_KNEE, "ENG": C_SKY, "POR": C_JUMP, "NED": C_RUN, "BEL": C_KNEE, "CRO": C_BELLY, "SWE": C_SKY, "ROU": C_JUMP, "NOR": C_SKY,
         "MAR": C_PRAY, "EGY": C_PRAY, "NGA": C_FLIP, "SEN": C_DANCE, "CMR": C_HIPS, "GHA": C_SAMBA, "JPN": C_RUN, "KOR": C_SKATE, "AUS": C_HIPS, "KSA": C_PRAY,
         "IRN": C_PRAY, "QAT": C_PRAY,
         "CHN": C_RUN, "DEN": C_KNEE, "SUI": C_SKY, "POL": C_JUMP, "SRB": C_SCREAM, "TUR": C_FLIP, "SCO": C_DANCE, "CZE": C_RUN,
         "BUL": C_SCREAM, "GRE": C_DANCE, "CHI": C_SCREAM, "PER": C_DANCE, "PAR": C_SKY, "ECU": C_DANCE, "CIV": C_DANCE, "ALG": C_JUMP}

# What the supporters wave: each side's flag in its two main colours (the stands paint them in
# key colours that the game sets to these at kick-off, cx_crowd_kits).
FLAGCOL = {
    "BRA": (0x009C3B, 0xFEDF00), "ARG": (0x74ACDF, W), "URU": (0x0038A8, W), "COL": (0xFCD116, 0x003893), "MEX": (0x006847, 0xCE1126),
    "USA": (0xB22234, 0x3C3B6E), "CAN": (0xD80621, W), "CRC": (0x002B7F, 0xCE1126), "FRA": (0x0055A4, 0xEF4135), "GER": (K, 0xFFCE00),
    "ITA": (0x009246, 0xCE2B37), "ESP": (0xAA151B, 0xF1BF00), "ENG": (W, 0xCE1124), "POR": (0x006600, 0xFF0000), "NED": (0xF07020, 0x21468B),
    "BEL": (K, 0xFDDA24), "CRO": (0xFF0000, W), "SWE": (0x006AA7, 0xFECC00), "ROU": (0x002B7F, 0xFCD116), "NOR": (0xBA0C2F, 0x00205B),
    "MAR": (0xC1272D, 0x006233), "EGY": (0xCE1126, K), "NGA": (0x008751, W), "SEN": (0x00853F, 0xFDEF42), "CMR": (0x007A5E, 0xCE1126),
    "GHA": (0xCE1126, 0xFCD116), "JPN": (W, 0xBC002D), "KOR": (W, 0xC60C30), "AUS": (0x00843D, 0xFFCD00), "KSA": (0x006C35, W),
    "IRN": (0x239F40, 0xDA0000), "QAT": (0x8A1538, W), "CHN": (0xDE2910, 0xFFDE00), "DEN": (0xC8102E, W), "SUI": (0xD52B1E, W),
    "POL": (W, 0xDC143C), "SRB": (0xC6363C, 0x0C4076), "TUR": (0xE30A17, W), "SCO": (0x005EB8, W), "CZE": (0xD7141A, 0x11457E),
    "BUL": (0x00966E, 0xD62612), "GRE": (0x0D5EAF, W), "CHI": (0xD52B1E, 0x0039A6), "PER": (0xD91023, W), "PAR": (0xD52B1E, 0x0038A8),
    "ECU": (0xFFD100, 0x034EA2), "CIV": (0xF77F00, 0x009E60), "ALG": (0x006233, W),
}

# colours as (dark, mid, light): skin, then hair
SKINS = [((196, 132, 100), (240, 186, 150), (254, 222, 192)),
         ((180, 112, 78), (232, 168, 124), (250, 206, 164)),
         ((150, 92, 62), (206, 142, 100), (236, 184, 140)),
         ((104, 58, 38), (150, 92, 62), (190, 128, 92)),
         ((62, 36, 28), (100, 62, 44), (140, 92, 68)),
         ((200, 142, 96), (242, 194, 148), (255, 228, 192))]      # warm light
HAIRS = [((24, 24, 36), (58, 62, 86), (110, 120, 150)),      # black
         ((44, 24, 18), (82, 48, 30), (124, 78, 50)),         # dark brown
         ((88, 50, 24), (138, 84, 40), (186, 126, 66)),       # brown
         ((120, 50, 20), (178, 84, 32), (222, 132, 60)),      # auburn
         ((168, 112, 26), (226, 176, 56), (248, 226, 124)),   # blond
         ((196, 160, 70), (240, 214, 120), (252, 244, 196))]  # fair


def word(rgb):
    """A colour as the Neo Geo's palette word (5 bits a channel, no dark bit)."""
    r, g, b = (min(31, c >> 3) for c in rgb)
    return ((r & 1) << 14) | ((g & 1) << 13) | ((b & 1) << 12) | ((r >> 1) << 8) | ((g >> 1) << 4) | (b >> 1)


def rgb_of(v):
    return (v >> 16) & 255, (v >> 8) & 255, v & 255


def ramp(v, n):
    """A team colour's shades, dark first: n=3 (dark, mid, light) or n=2."""
    r, g, b = rgb_of(v)
    dark = (int(r * 0.58), int(g * 0.58), int(b * 0.62))
    light = (min(255, r + (255 - r) * 2 // 5 + 8), min(255, g + (255 - g) * 2 // 5 + 8), min(255, b + (255 - b) * 2 // 5 + 8))
    if max(r, g, b) < 40:                       # black: the shades must still show
        dark, mid, light = (8, 8, 12), (r + 14, g + 14, b + 18), (72, 76, 90)
        return (dark, mid, light) if n == 3 else (dark, mid)
    if min(r, g, b) > 200:                      # white: blue-grey folds
        dark, light = (150, 158, 184), (255, 255, 255)
    return (dark, (r, g, b), light) if n == 3 else (dark, (r, g, b))


def kit_words(kit):
    shirt, shorts, socks = kit
    return [word(c) for c in ramp(shirt, 3)] + [word(c) for c in ramp(shorts, 2)] + [word(c) for c in ramp(socks, 2)]


def clash(a, b):
    ra, rb = rgb_of(a), rgb_of(b)
    return sum(abs(x - y) for x, y in zip(ra, rb))


def roster(n, region):
    """Sixteen men: goalkeeper first, 10 starters, 5 reserves; name and attributes."""
    rng = random.Random(0x787000 + n)
    pool = [s for s in NAMES[region] if s not in AVOID]
    rng.shuffle(pool)
    speed, power, press, control, defence, keeper = NATIONS[n][4]
    skins, _ = LOOKS[region]
    hairs = HAIR_BY_REGION[region]
    out = []
    for i in range(16):
        line = 0 if i == 0 or i == 11 else (1 if i < 5 or i in (12, 13) else (2 if i < 9 or i == 14 else 3))
        star = 2 if i in (8, 9, 10) else 0
        def at(v, bonus=0):
            return max(1, min(16, v + bonus + rng.randint(-2, 1)))
        pace = at(speed, (0, 0, 0, 1)[line] + star // 2)
        shot = at(power, (-4, -3, 0, 2)[line] + star // 2)
        ctrl = at(control, (-3, -2, 1, 1)[line] + star // 2)
        tack = at(defence, (0, 2, 0, -3)[line])
        fig = rng.choice(FIG_BY_REGION.get(region, (0, 0, 1, 2, 2, 3, 4 if max(skins) >= 3 else 2)))
        skin = rng.choice(skins)
        hair = rng.choice(hairs)
        if fig in (0, 4) and hair > 1:     # these cuts have dark hair drawn into their outline
            hair = rng.choice((0, 1))
        if fig == 4:
            skin = max(skin, 3)
        kind = (T_PLAIN, T_STOPPER if tack > 12 else T_PLAIN, T_PLAYMAKER if ctrl > 13 else (T_DRIBBLE if pace > 13 else T_PLAIN),
                T_SPEED if pace > 13 else (T_TARGET if shot > 13 else (T_SHOOTER if shot > 11 else T_PLAIN)))[line]
        look = STARS.get((NATIONS[n][1], i))
        if look:                           # the side's famous-looking man, by type and never by name
            fig, skin, hair, kind = look
            pace, shot, ctrl = min(16, pace + 1), min(16, shot + 1), min(16, ctrl + 2)
            if kind == T_SPEED: pace = min(16, pace + 2)
            elif kind == T_SHOOTER: shot = min(16, shot + 2)
            elif kind in (T_DRIBBLE, T_PLAYMAKER): ctrl = min(16, ctrl + 2)
            elif kind == T_STOPPER: tack = min(16, tack + 3)
        # his build: tall (drawn a size up) for the big men at the back and up front, and
        # where sides run tall; slight (a size down) for some quick small men. Its own dice,
        # so the rest of the roster is as it was.
        brng = random.Random(0xB111D + n * 16 + i)
        build = 0
        r = brng.random()
        if region in TALL and (kind == T_TARGET or i in (0, 11) or (kind == T_STOPPER and r < 0.5) or (line == 1 and r < 0.2)):
            build = 1
        elif kind == T_TARGET or (kind == T_STOPPER and r < 0.35):
            build = 1
        elif (region in SLIGHT and line >= 2 and r < 0.6) or (kind == T_SPEED and r < 0.3):
            build = 2
        out.append((pool[i % len(pool)][:10], pace, shot, ctrl, tack, fig, skin, hair, kind, build))
    return out


TALL = {"nl", "de", "no", "dk", "rs", "hr", "cz", "pl", "ng", "sn", "gh", "ci"}
SLIGHT = {"jp", "kr", "cn"}


VOICE_V1 = (Path(__file__).resolve().parent / "voice_names_v1.txt").read_text().split()
CHANTS_V1 = 32


def voice_names():
    """Every shirt name in every squad, once, in a fixed order: the commentator's
    recordings of them (tools/voices.py) are numbered by it. The first 316 were recorded
    before the 48 sides and keep their numbers (voice_names_v1.txt); new ones follow."""
    s = set()
    for i, n in enumerate(NATIONS):
        for m in roster(i, n[2]):
            s.add(m[0])
    return VOICE_V1 + sorted(s - set(VOICE_V1))


def header():
    q = ["/* Generated by games/centuryxi/tools/art.py from tools/nations.py: do not edit. */",
         "#ifndef CX_DATA_H", "#define CX_DATA_H", "#include <stdint.h>", f"#define CX_NATIONS {len(NATIONS)}", "#define CX_SQUAD_MEN 16",
         f"#define CX_CHANTS_V1 {CHANTS_V1}", f"#define CX_VOICE_NAMES_V1 {len(VOICE_V1)}"]
    q.append("static const char cx_nation_name[CX_NATIONS][14] = {" + ",".join(f'"{shown(n)[0]}"' for n in NATIONS) + "};")
    q.append("static const char cx_nation_code[CX_NATIONS][4] = {" + ",".join(f'"{shown(n)[1]}"' for n in NATIONS) + "};")
    q.append("static const uint8_t cx_nation_form[CX_NATIONS] = {" + ",".join(str(n[3]) for n in NATIONS) + "};")
    q.append("/* speed, power, press, control, defence, keeper */")
    q.append("static const uint8_t cx_nation_stat[CX_NATIONS][6] = {" + ",".join("{" + ",".join(map(str, n[4])) + "}" for n in NATIONS) + "};")
    q.append("/* shirt dark mid light, shorts dark mid, socks dark mid: [nation][home, away, keeper] */")
    rows = []
    for n in NATIONS:
        gk = (n[7], K if min(rgb_of(n[7])) > 40 else W, n[7])
        rows.append("{" + ",".join("{" + ",".join(f"0x{v:04X}u" for v in kit_words(k)) + "}" for k in (n[5], n[6], gk)) + "}")
    q.append("static const uint16_t cx_nation_kit[CX_NATIONS][3][7] = {" + ",".join(rows) + "};")
    q.append("/* the shirt's colour, for the clash test: home, away */")
    q.append("static const uint8_t cx_nation_rgb[CX_NATIONS][2][3] = {" +
             ",".join("{{%d,%d,%d},{%d,%d,%d}}" % (rgb_of(n[5][0]) + rgb_of(n[6][0])) for n in NATIONS) + "};")
    q.append("/* 1: the shirt is light, so numbers go on it in dark: [nation][home, away, keeper] */")
    light = lambda v: 1 if (0.3 * ((v >> 16) & 255) + 0.59 * ((v >> 8) & 255) + 0.11 * (v & 255)) > 150 else 0
    q.append("static const uint8_t cx_nation_ink[CX_NATIONS][3] = {" + ",".join("{%d,%d,%d}" % (light(n[5][0]), light(n[6][0]), light(n[7])) for n in NATIONS) + "};")
    q.append(f"static const uint16_t cx_skin_pal[{len(SKINS)}][3] = {{" + ",".join("{" + ",".join(f"0x{word(c):04X}u" for c in s) + "}" for s in SKINS) + "};")
    q.append(f"static const uint16_t cx_hair_pal[{len(HAIRS)}][3] = {{" + ",".join("{" + ",".join(f"0x{word(c):04X}u" for c in s) + "}" for s in HAIRS) + "};")
    names, stats = [], []
    for i, n in enumerate(NATIONS):
        r = roster(i, n[2])
        names.append("{" + ",".join(f'"{m[0]}"' for m in r) + "}")
        stats.append("{" + ",".join("{" + ",".join(str(v) for v in m[1:]) + "}" for m in r) + "}")
    q.append("static const char cx_roster_name[CX_NATIONS][CX_SQUAD_MEN][11] = {" + ",".join(names) + "};")
    q.append("/* pace, shot, control, tackle, figure, skin, hair, type, build (0 as drawn, 1 tall, 2 slight) */")
    q.append("static const uint8_t cx_roster_stat[CX_NATIONS][CX_SQUAD_MEN][9] = {" + ",".join(stats) + "};")
    vn = {name: k for k, name in enumerate(voice_names())}
    q.append(f"#define CX_VOICE_NAMES {len(vn)}")
    q.append("/* each man's name as the commentator says it: an index into tools/voices.py's recordings */")
    q.append("static const uint16_t cx_roster_voice[CX_NATIONS][CX_SQUAD_MEN] = {" +
             ",".join("{" + ",".join(str(vn[m[0]]) for m in roster(i, n[2])) + "}" for i, n in enumerate(NATIONS)) + "};")
    q.append("static const uint8_t cx_nation_style[CX_NATIONS] = {" + ",".join(str(STYLE[n[1]]) for n in NATIONS) + "};")
    q.append("static const uint8_t cx_nation_celeb[CX_NATIONS] = {" + ",".join(str(CELEB[n[1]]) for n in NATIONS) + "};")
    q.append("/* the supporters' flags: main colour, second colour (palette words) */")
    q.append("static const uint16_t cx_nation_flagcol[CX_NATIONS][2] = {" +
             ",".join("{0x%04Xu,0x%04Xu}" % tuple(word(rgb_of(c)) for c in FLAGCOL[n[1]]) for n in NATIONS) + "};")
    q += ["#endif", ""]
    return "\n".join(q)


if __name__ == "__main__":
    print(len(NATIONS), sum(len(header()) for _ in [0]), "bytes of header")
    for i, n in enumerate(NATIONS[:3]):
        print(n[0], [m[0] for m in roster(i, n[2])][:6])

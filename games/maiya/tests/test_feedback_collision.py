"""Exercise Maiya's real ledge/attack helpers with gameplay boundary cases."""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]


class CollisionFeedbackTests(unittest.TestCase):
    def test_ledge_edges_and_attacks_on_different_tiers(self):
        compiler = shutil.which('cc')
        if not compiler:
            self.skipTest('Host C compiler unavailable')
        source = r'''
#include <assert.h>
#include "maiya_collision.h"
int main(void) {
    /* Four pixels of boot overlap on either edge; further out falls. */
    assert(mg_ledge_reaches(96,100,64));
    assert(mg_ledge_reaches(168,100,64));
    assert(!mg_ledge_reaches(95,100,64));
    assert(!mg_ledge_reaches(169,100,64));
    assert(!mg_ledge_reaches(100,100,0));
    assert(mg_ledge_reaches(-12,-8,32));
    /* Odd half-tile widths used to lose a visible 16-pixel cap section. */
    assert(mg_ledge_blocks(48)==2);
    assert(mg_ledge_blocks(80)==3);
    assert(mg_ledge_blocks(96)==3);
    assert(mg_ledge_reaches(168,100,48));
    assert(!mg_ledge_reaches(169,100,48));
    /* A ground creature occupies feet-24 through feet. */
    assert(mg_whip_height(192,0,168,24));
    assert(mg_whip_height(192,1,168,24));
    assert(!mg_whip_height(112,0,168,24));
    assert(!mg_whip_height(112,1,168,24));
    assert(!mg_whip_height(192,0,80,24));
    /* Kneel to reach a low crawler; standing does not hit empty air. */
    assert(!mg_whip_height(192,0,184,8));
    assert(mg_whip_height(192,1,184,8));
    /* An overhead creature whose body crosses the whip still takes a hit. */
    assert(mg_whip_height(192,0,150,24));
    /* Tall guardians remain reachable, but not from an arbitrary high tier. */
    assert(mg_whip_height(192,0,116,76));
    assert(!mg_whip_height(80,0,116,76));
    assert(mg_attack_height(144,48,116,76));
    assert(!mg_attack_height(16,48,116,76));
    /* Touching boundaries alone are not overlapping hurtboxes. */
    assert(!mg_attack_height(100,12,112,24));
    assert(mg_attack_height(100,13,112,24));
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='maiya-feedback-') as folder:
            path = Path(folder)
            (path/'check.c').write_text(source)
            subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-I', str(GAME/'scenes'), str(path/'check.c'),
                            '-o', str(path/'check')], check=True)
            subprocess.run([str(path/'check')], check=True)


if __name__ == '__main__':
    unittest.main()

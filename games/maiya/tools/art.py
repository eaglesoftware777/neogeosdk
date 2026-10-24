"""Install Maiya's approved V4 graphics without editing or regenerating art."""
import argparse
from locked_art import install


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fix-only', action='store_true')
    parser.parse_args()
    # Both make targets use the same immutable approved source banks.
    install()
    print('Approved V4 graphics and FIX installed; artwork generation is disabled')


if __name__ == '__main__':
    main()

#!/usr/bin/env python3.6

import argparse

import aegir

parser = argparse.ArgumentParser('Aegir API')
parser.add_argument('-c', '--config', dest='configfile',
                    default='/usr/local/etc/aegir/api.yaml',
                    help='Path to the configfile')
args = parser.parse_args()


if __name__ == '__main__':
    aegir.init(args.configfile)
    aegir.run()

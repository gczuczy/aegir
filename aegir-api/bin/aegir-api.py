#!/usr/bin/env python3.8

import argparse

import aegir

parser = argparse.ArgumentParser('Aegir API')
parser.add_argument('-c', '--config', dest='configfile',
                    default='/usr/local/etc/aegir/api.yaml',
                    help='Path to the configfile')
args = parser.parse_args()
aegir.init(args.configfile)


if __name__ == '__main__':
    aegir.run()

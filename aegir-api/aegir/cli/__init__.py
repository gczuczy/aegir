'''
CLI and WSGI entrypoints
'''

import argparse
import aegir

def parse_args():
    parser = argparse.ArgumentParser('Aegir API')
    parser.add_argument('-c', '--config', dest='configfile',
                        default='/usr/local/etc/aegir/api.yaml',
                        help='Path to the configfile')
    parser.add_argument('-d', '--debug', dest='debug',
                        default=False,
                        action=argparse.BooleanOptionalAction,
                        help='Path to the configfile')
    args = parser.parse_args()
    return args

def api():
    args = parse_args()
    aegir.init(args.configfile)
    runargs = {'debug': args.debug}
    aegir.run(**runargs)
    pass

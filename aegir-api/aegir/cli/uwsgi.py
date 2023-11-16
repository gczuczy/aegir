'''
WSGI entrypoint
'''

import argparse
import aegir
from aegir.cli import parse_args

args = parse_args()

application = aegir.init(args.configfile)

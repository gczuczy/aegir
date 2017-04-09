# reading the configuration file and storing the config

import yaml
from pprint import pprint

config = {}

def init(cfgfile):
    global config
    with open(cfgfile, 'r') as cfgfh:
        config = yaml.load(cfgfh.read())
        pass
    #pprint(config)
    pass

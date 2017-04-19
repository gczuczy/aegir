'''
Module's entry point for the API with the initialization calls
'''

import aegir.api.programs
import aegir.api.brewd

def init(app, api):
    aegir.api.programs.init(app, api)
    aegir.api.brewd.init(app, api)
    pass

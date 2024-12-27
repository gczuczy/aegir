'''
Module's entry point for the API with the initialization calls
'''

import aegir.api.programs
import aegir.api.brewd
import aegir.api.fermd

def init(app, api):
    aegir.api.programs.init(app, api)
    aegir.api.brewd.init(app, api)
    aegir.api.fermd.init(app, api)
    pass

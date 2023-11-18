__all__ = ['binaries', 'plotting', 'testing']

def setup() -> None :

    import sys
    
    is_submodule = lambda x : False if x.startswith('__') or x == 'setup' else True
    submodules = [ module for module in dir(sys.modules[__name__]) if is_submodule(module) ]

    print('**** MODULES IN GLOBAL NAMESPACE ****\n')

    for submodule in submodules:                                    # this gets the relevant modules in
        sys.modules[f'{__name__}.{submodule}'].setup()              # the appropriate (global) namespace
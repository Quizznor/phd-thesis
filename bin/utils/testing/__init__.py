from .. import create_stream_logger

testing_logger = create_stream_logger('utils.testing ')
import timeit
testing_logger.info('import timeit')

timeit.template = """
def inner(_it, _timer{init}):
    {setup}
    _t0 = _timer()
    for _i in _it:
        retval = {stmt}
    _t1 = _timer()
    return _t1 - _t0, retval
"""
testing_logger.debug('overwrite timeit.template to catch return values')

from . import testing_tools as testing
testing_logger.info('import testing.tools as testing')
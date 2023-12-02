__all__ = ['binaries', 'plotting', 'testing', 'Auger']

import logging
from colorlog import ColoredFormatter
import re

class Formatter(ColoredFormatter):
    """Custom logging class to allow for stylized formatting, and runtime analysis"""

    last_log = 0
    levels = {
    'DEBUG' : "DBUG",
    "INFO" : "INFO",
    "WARNING" : "WARN",
    "ERROR" : "ERR",
    "CRITICAL" : "CRIT"}

    def format(self, record):
        delta_milliseconds = record.relativeCreated - Formatter.last_log
        Formatter.last_log = int(record.relativeCreated)
        record.threadName = f"+{int(delta_milliseconds)}ms" if delta_milliseconds < 99999 else f'<99999ms'
        record.threadName = record.threadName.rjust(8)
        record.thread = f"{f'[{self.levels[record.levelname]}]': <6}"
        record.msg = self.format_message(record.msg)

        return super().format(record)	

    @staticmethod
    def format_message(msg : str) -> str :
        """filter and format specific keywords in logging messages"""
        BOLD = '\033[1m'
        FAIL = '\033[91m'
        ENDC = '\033[0m'

        import_as_pattern = re.compile('import\s.*as\s\S*')
        import_pattern = re.compile('import\s\S*')
        if re.search(import_as_pattern, msg):
            _import, _module, _as, _alias = msg.split()
            _module = BOLD + FAIL + _module + ENDC
            _alias = BOLD + FAIL + _alias + ENDC
            msg = ' '.join([_import, _module, _as, _alias])
        elif re.search(import_pattern, msg):
            _import, *_modules = msg.split()
            _module = ''
            for _pkg in _modules:
                _pkg = _pkg.replace(',', '')
                _module += BOLD + FAIL + _pkg + ENDC
            if not _pkg == _modules[-1]:
                _module += ', '
            msg = ' '.join([_import, _module])
        
        return msg

    def __init__(self) -> None :
        super().__init__(
            f'{{log_color}}{{asctime}} {{thread}} {{name}}{{reset}} -- ({{bold_purple}}{{threadName}}{{reset}}) -- {{msg}}',
            datefmt='%H:%M:%S',
            reset=True,
            log_colors={
            'DEBUG' :		'bold_cyan',
            'INFO' :    	'bold_green',
            'WARNING' : 	'bold_yellow',
            'ERROR' :   	'bold_red',
            'CRITICAL' :	'bold_blue',
            },
            secondary_log_colors={},
            style='{'
        )

def create_stream_logger(name : str, loglevel=logging.DEBUG) -> logging.Logger :
    """create a simple logger that streams to stdout"""

    logger = logging.getLogger(name)
    logger.setLevel(loglevel)
    ch = logging.StreamHandler()
    ch.setLevel(0)
    ch.setFormatter(Formatter())
    logger.addHandler(ch)

    return logger

utils_logger = create_stream_logger('utils         ')
utils_logger.info('import logging')
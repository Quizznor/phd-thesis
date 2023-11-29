import colorlog, logging
import re

class Formatter(colorlog.ColoredFormatter):
	"""Custom logging class to allow for stylized formatting, and runtime analysis"""

	last_log = 0
	levels = {
		'DEBUG' : "DBUG",
		"INFO" : "INFO",
		"WARNING" : "WARN",
		"ERROR" : "ERR",
		"CRITICAL" : "CRIT"}

	def format(self, record):
		delta_milliseconds = record.relativeCreated - self.last_log
		self.last_log = int(record.relativeCreated)
		record.relativeCreated = f"+{int(delta_milliseconds)}ms" if self.last_log != 0 else f"!!!!"
		record.levelno = f"{f'[{self.levels[record.levelname]}]': <6}"
		record.msg = self.format_message(record.msg)

		return super().format(record)	
	
	@staticmethod
	def format_message(msg : str) -> str :
		
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
			return f"{msg: <126}"
		elif re.search(import_pattern, msg):
			_import, *_modules = msg.split()
			_module = ''
			padding = 100

			for _pkg in _modules:
				_pkg = _pkg.replace(',', '')
				_module += BOLD + FAIL + _pkg + ENDC
				padding += 13
				if not _pkg == _modules[-1]:
					_module += ', '
			msg = ' '.join([_import, _module])
			return f"{msg: <{padding}}"
		else:
			return f"{msg: <100}"

	def __init__(self) -> None :
		super().__init__(
			f'{{log_color}}{{asctime}} {{levelno}} {{name}}{{reset}} -- {{msg}} ({{bold_purple}}{{relativeCreated}}{{reset}})',
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
            style='{')

def create_stream_logger(name : str, loglevel=logging.DEBUG) -> logging.Logger :
	"""create a simple logger that streams to stdout"""

	logger = logging.getLogger(name)
	logger.setLevel(loglevel)
	ch = logging.StreamHandler()
	ch.setLevel(0)
	ch.setFormatter(Formatter())
	logger.addHandler(ch)

	return logger

testing_logger = create_stream_logger('utils.testing ')
testing_logger.info('import colorlog, logging')
testing_logger.info('import re')
from . import tools as testing
testing_logger.info('import testing.tools as testing')
del tools
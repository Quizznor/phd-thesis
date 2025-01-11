__all__ = ["binaries", "plotting", "Auger"]

from termcolor import colored
import logging
import time
import re



class Formatter(logging.Formatter):
    """Custom logging class to allow for stylized formatting, and (rough) runtime analysis"""

    last_log = 0

    def __init__(self) -> None:
        super().__init__()

    def format(self, record: logging.LogRecord) -> str:

        format_time = lambda t: f"+{int(t)}ms" if t < 1000 else f"+{t/1000:.1f}s"
        lvl = record.levelno // 10 - 1

        delta_milliseconds = record.relativeCreated - Formatter.last_log
        Formatter.last_log = int(record.relativeCreated)
        elapsed = format_time(delta_milliseconds).rjust(8)
        threat = f"[{record.levelname: <7}]"
        localtime = time.strftime("%H:%M:%S", time.localtime())
        msg = self.highlight_imports(record.msg)

        return f"{self.color(lvl, localtime)} ({self.color(-1, elapsed)}) {self.color(lvl, threat)} -- {msg}"

    def highlight_imports(self, msg: str) -> str:
        """filter and format specific keywords in logging messages"""

        import_as_pattern = re.compile("import\s.*as\s\S*")
        import_pattern = re.compile("import\s\S*")
        if re.search(import_as_pattern, msg):
            _import, _module, _as, _alias = msg.split()
            _module = self.color(3, _module)
            _alias = self.color(3, _alias)
            msg = " ".join([_import, _module, _as, _alias])
        elif re.search(import_pattern, msg):
            _import, *_modules = msg.split()
            _module = ""
            for _pkg in _modules:
                _pkg = _pkg.replace(",", "")
                _module += self.color(3, _pkg)
            if not _pkg == _modules[-1]:
                _module += ", "
            msg = " ".join([_import, _module])

        return msg

    @staticmethod
    def color(lvl: int, msg: str) -> str:

        colors = ["blue", "green", "yellow", "red", "red", "magenta"]
        return colored(msg, colors[lvl], attrs=["bold"])


def create_stream_logger(name: str, loglevel: int = logging.DEBUG) -> logging.Logger:
    """create a simple logger that streams to stdout"""

    logger = logging.getLogger(f"{name:<14}")

    if not logger.hasHandlers():
        logger.setLevel(loglevel)
        ch = logging.StreamHandler()
        ch.setLevel(0)
        ch.setFormatter(Formatter())
        logger.addHandler(ch)

    return logger


utils_logger = create_stream_logger("utils")
utils_logger.info("import logging")

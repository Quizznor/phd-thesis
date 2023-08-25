import numpy as np

class ADCTrace():

    def _init__(self, *args, **kwargs) -> None :

        self.trace_length = 2048

        pass

    def __iter__(self) -> "ADCTrace" :
        self.__iteration_index = -120
        self.__window_length = - self.__iteration_index
        return self
    
    def __next__(self) -> list :

        if self.__iteration_index >= self.trace_length:
            raise StopIteration

        self.__iteration_index += self.__window_length
        try:
            return self.trace[self.__iteration_index : self.__iteration_index + self.__window_length]
        except IndexError:
            return self.trace[self.__iteration_index:]


class OnlineAlgorithm():


    def __init__(self, **kwargs) -> None :
        pass

    def __call__() -> int : 
        pass
from threading import Condition, Lock


class event(object):
    def __init__(self, events):
        self.conds = dict()
        for evt in events:
            self.conds[evt] = Condition(Lock())
        self.data = None


    def wait(self, evt, timeout = 5):
        self.data = None
        with self.conds[evt]:
            self.conds[evt].wait(timeout=timeout)
            return self.data


    def notify(self, evt, data = None):
        with self.conds[evt]:
            self.data = data
            self.conds[evt].notify_all()
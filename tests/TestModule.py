class TestClass:
    def returnInt(self):
        return 1113

    def returnString(self):
        return "Test String"

    def returnTuple(self):
        return (1, "two", 3.0)

    def returnList(self):
        return [1, "two", 3.0]

    def returnDict(self):
        d = {}
        d['one'] = 1
        d['two'] = 2
        d['three'] = 3
        return d

    def returnSelf(self):
        return self

    def returnNone(self):
        return None

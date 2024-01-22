'''
objects.k3s implemntation. Produces dump in a form that can be compared with
k3s array dump via diff.
'''

def dumpInt(x, i, depth):
    print(("-" * depth) + f" [{i}] {{ type_: NUM, val_: {float(x):.6f}}}")

def dumpNone(i, depth):
    print(("-" * depth) + f" [{i}] {{ type_: ANY, val_: 0}}")

class Bar:
    def __init__(self, a):
        self.a = a
    
    def dump(self, i, depth):
        size = size = len(self.__dict__)
        size += 1 # constructor
        print(("-" * depth) + f" [{i}] {{ type_: OBJ, size_: {size}; data_:")
        dumpInt(self.a, i=0, depth=depth + 1)
        print("-" * (depth + 1) + "}")

class Foo:
    def __init__(self, x):
        self.x = x
        self.y = None

    def setY(self, b):
        self.y = b

    def dump(self, i):
        size = len(self.__dict__)
        size += 1 # constructor
        size += 1 # setY
        print(f" [{i}] {{ type_: OBJ, size_: {size}; data_:")
        self.dumpFields()
        print("-}")

    def dumpFields(self):
        i = 0
        for key in self.__dict__:
            obj = self.__dict__[key]
            if type(obj) == Bar:
                obj.dump(i, depth=1)
            elif type(obj) == int:
                dumpInt(obj, i, depth=1)
            else:
                assert(obj == None)
                dumpNone(i, depth=1)
            i += 1

def dump(arr):
    print("{{ type_: ARR, size_: {sz}; data_:".format(sz = len(arr)))
    i = 0
    for el in arr:
        el.dump(i)
        i += 1
    print("}")

def func(N, M):
    foo = [None] * M
    outer = None
    for i in range(1, N + 1):
        o1 = Foo(i)
        if (i % 3 == 0):
            foo[i % M] = o1
            # foo[i % M - 1] = o1
        o2 = Bar(i)
        if (i % 5 == 0):
            o1.setY(o2)
        outer = o1
    dump(foo)

def main():
    N = 4000000
    M = 1000
    func(N, M)

main()
with open('out.txt') as f:
    s = 0
    for line in f:
        for word in line.split():
            s += 1
print(s)

with open("GATEWAY/src/main.cpp", "r") as f:
    c = f.read()
c = c.replace("Math.atan2", "atan2")
c = c.replace("Math.sqrt", "sqrt")
c = c.replace("Math.PI", "PI")
with open("GATEWAY/src/main.cpp", "w") as f:
    f.write(c)

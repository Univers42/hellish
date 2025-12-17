import shlex
command = 'python script.py --name "Dylan the dev" --age 26'
parts = shlex.split(command)
print(parts)
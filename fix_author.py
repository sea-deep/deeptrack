import json

diagram = json.load(open('diagram.json'))
diagram['author'] = ""
json.dump(diagram, open('diagram.json', 'w'), indent=2)

for chip in ['vl53l0x', 'tb6612fng']:
    with open(f'{chip}.chip.json', 'r') as f:
        data = json.load(f)
    data['author'] = ""
    json.dump(data, open(f'{chip}.chip.json', 'w'), indent=2)

import json

diagram = json.load(open('diagram.json'))
print([p['type'] for p in diagram['parts']])

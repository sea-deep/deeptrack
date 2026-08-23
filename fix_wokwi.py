import json

for chip in ['vl53l0x', 'tb6612fng']:
    with open(f'{chip}.chip.json', 'r') as f:
        data = json.load(f)
    
    # name must be the exact suffix of 'chip-'
    data['name'] = chip 
    
    with open(f'{chip}.chip.json', 'w') as f:
        json.dump(data, f, indent=2)


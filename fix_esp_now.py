import re

def update_file(filename):
    with open(filename, "r") as f:
        content = f.read()

    # Add the toggle flag at the top
    if "#define USE_ESP_NOW" not in content:
        content = re.sub(r'(#include <WiFi\.h>\n#include <esp_now\.h>)', r'\1\n\n#define USE_ESP_NOW 0\n', content)
    
    # Wrap esp_now_init block
    content = re.sub(r'(\s+if \(esp_now_init\(\) != ESP_OK\) \{.*?\s+esp_now_add_peer\(&peerInfo\);\n)', 
                     r'\n#if USE_ESP_NOW\1#endif\n', content, flags=re.DOTALL)
                     
    # Wrap esp_now_send calls
    content = re.sub(r'(\s+esp_now_send\([^;]+;\n)', r'\n#if USE_ESP_NOW\1#endif\n', content)
    
    with open(filename, "w") as f:
        f.write(content)

update_file("src/main.cpp")
update_file("GATEWAY/src/main.cpp")

"""
Simple Python based demonstration
"""
import requests
import time

# Put your device IP address here
url = "http://192.168.1.170"

myobj = {"c": "hello"}
response = requests.post(url, data=myobj)
print(response.text)

myobj = {"r": "0", "g": "0", "b": "0"}
response = requests.post(url, data=myobj)
time.sleep(2.0)
all_led_colors = [10, 20, 30, 40, 50, 60, 70]
all_leds_testing = [0, 5, 10, 15, 20, 25, 20]
for color in all_led_colors:
    for led in all_leds_testing:
        myobj = {
            "r" + str(led): str(color),
            "g" + str(led): str(color),
            "b" + str(led): str(color),
        }
        response = requests.post(url, data=myobj)
        print("Returned: ", response.status_code)

myobj = {"c": "hello"}
response = requests.post(url, data=myobj)

myobj = {"s3": "hello"}
response = requests.post(url, data=myobj)

myobj = {"r": "5", "g": "5", "b": "5"}
response = requests.post(url, data=myobj)

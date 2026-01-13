import requests

response = requests.get("http://192.168.4.1:8000")
print(response.status_code)
print(response.text)
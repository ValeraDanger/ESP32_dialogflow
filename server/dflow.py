import requests
import base64
import json
import os



def df_request (audio):
    os.environ['GOOGLE_APPLICATION_CREDENTIALS'] = "gcloud_app_cred.json"
    url = "https://dialogflow.googleapis.com/v2/projects/smartsarov-suae/agent/sessions/123456789:detectIntent"
    token = os.popen("gcloud auth application-default print-access-token").read()[:-1]
    body = '{ "queryInput": { "audioConfig": { "languageCode": "ru-RU" } }, "inputAudio": "%s" }' % (audio.decode('utf-8'))
    header = {"Authorization": "Bearer {}".format(token)}
    req = requests.post(url, data=body, headers = header)
    json_response = json.loads(req.content.decode("utf-8"))
    print(json_response)
    is_succsess = "queryResult" in json_response
    return is_succsess, json_response
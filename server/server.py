from flask import Flask, request, jsonify, json
import wave, time, dflow, base64

adc_raw_name = 'adc.raw'
i2s_raw_name = 'i2s.raw'
esp32_wav_name = 'esp32.wav'

app = Flask(__name__)

allowWrite = True

def raw_to_wav(raw_name):
    with open(raw_name, "rb") as inp_f:
        data = inp_f.read()
        with wave.open(esp32_wav_name, "wb") as out_f:
            out_f.setnchannels(1)
            out_f.setsampwidth(2)  # number of bytes
            out_f.setframerate(16000)
            out_f.writeframesraw(data)


@app.route('/esp32', methods = ['POST'])
def esp32():
    print(request.headers)
    print('------------')
    #print(request.data)
    #print('---------')
    print(request)
    resp_json = {"text": "Ответа нет :("}
    with open("test.raw", 'wb') as file:
        file.write(request.get_data())

    try:
        raw_to_wav("test.raw")
        with open(esp32_wav_name, 'rb') as file:
            message = file.read()
            print('Запрос к dialogflow...')
            is_succsess, json_response = dflow.df_request(base64.b64encode(message))
            response = json_response["queryResult"]["fulfillmentText"]
            print(response)
            #dflowRequested = True
            resp_json = {"text": f"{response}"}
    except:
        print('Не удалось произвести запрос к dialogflow')

    return jsonify(resp_json)

@app.route('/adc_samples', methods=['POST'])
def adc_samples():
    with open(adc_raw_name, 'ab') as file:
        file.write(request.get_data())
        print('sample written')
    return "OK"

@app.route('/i2s_samples', methods=['POST'])
def i2s_samples():
    if allowWrite:
        with open(i2s_raw_name, 'ab') as file:
            file.write(request.get_data())
            print('sample written')
    return "OK"

@app.route('/btn_clicked', methods=['GET'])
def btn_clicked():
    print('button clicked!')
    global allowWrite
    allowWrite = not allowWrite
    return "OK"

if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True, port=5003)




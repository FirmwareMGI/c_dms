import ctypes
try:
    import lib61850
except:
    import lib61850_linux as lib61850
import zipfile
from zipfile import ZipFile
from libiec61850client import iec61850client
import db
import time
from datetime import datetime
import mqtt as mqtt
import requests

import dfr_parse

import os
import socket
import paho.mqtt.publish as mqtt_publish

DEBUG = False  # """untuk percobaan dengan server localhost"""

if DEBUG:
    dataDevice = ['127.0.0.1']
else:
    DBdataMeasurement = db.readDb.m_file_iec_read_by_active(0)
    DBdataDevices = db.readDb.device_list(1)
    DBdataNetwork = db.readDb.network(0)
    DBdataMMesin = db.readDb.m_mesin(0)
    # dataDRIEC = db.
dataDevice = []
dbMqtt = db.readDb.network_mqtt(0)

for v in DBdataDevices:
    if v['port_type'] == '2':
        dataDevice.append(v)
max_client = len(dataDevice)

dic_conn = []

alias_name = []
rack_location = []
alias_monit = []
ipserver = []
hostname = []
tcpport = []
error = []
con = []
model = []
object_read = []
object_FC = []
file_list = []
rack_location = []
type_relay = []
name_alias = []
disturbanceType = []
iecfolder = []

network_ipserver = DBdataNetwork[0]['ipserver']
mesin_mesin = DBdataMMesin[0]['kode_mesin']
mesin_namaGi = DBdataMMesin[0]['nama_gi']

#disturbanceType = DBdataDevices[0]['disturbance_type']
#print(disturbanceType)


client = None

extension_file_dr = [".cfg", ".dat", ".hdr", ".cev", ".txt",
                     ".CFG", ".DAT", ".HDR", ".CEV", ".TXT"]
mqttRetry = 0

def _FC(rawfc):
    if rawfc == "ST":
        return lib61850.IEC61850_FC_ST
    if rawfc == "MX":
        return lib61850.IEC61850_FC_MX
    if rawfc == "SP":
        return lib61850.IEC61850_FC_SP
    if rawfc == "SV":
        return lib61850.IEC61850_FC_SV
    if rawfc == "CF":
        return lib61850.IEC61850_FC_CF
    if rawfc == "DC":
        return lib61850.IEC61850_FC_DC
    if rawfc == "SG":
        return lib61850.IEC61850_FC_SG
    if rawfc == "SE":
        return lib61850.IEC61850_FC_SE
    if rawfc == "SR":
        return lib61850.IEC61850_FC_SR
    if rawfc == "OR":
        return lib61850.IEC61850_FC_OR
    if rawfc == "BL":
        return lib61850.IEC61850_FC_BL
    if rawfc == "EX":
        return lib61850.IEC61850_FC_EX
    if rawfc == "CO":
        return lib61850.IEC61850_FC_CO
    if rawfc == "US":
        return lib61850.IEC61850_FC_US
    if rawfc == "MS":
        return lib61850.IEC61850_FC_MS
    if rawfc == "RP":
        return lib61850.IEC61850_FC_RP
    if rawfc == "BR":
        return lib61850.IEC61850_FC_BR
    if rawfc == "LG":
        return lib61850.IEC61850_FC_LG
    if rawfc == "GO":
        return lib61850.IEC61850_FC_GO


def _getFileHandler(parameter, buffer, bytesRead):
    filename = ctypes.cast(parameter, ctypes.c_char_p).value.decode(
        "utf-8")  # convert parameter back to python string
    #print("received %i bytes for file: %s" % (bytesRead, filename))
    if bytesRead > 0:
        with open(filename, "ab") as fp:
            bytesbuffer = ctypes.string_at(buffer, bytesRead)
            fp.write(bytesbuffer)  # append bytes to file
    return True


# def ClientFile61850_dir():
def ClientFile61850_dir(con,iec_dir):
    counterFile = 0
    listFile = []
    # for i in range(0, max_client):
    #   if error[i]['error'].value == lib61850.IED_ERROR_OK:
    error_file = lib61850.IedClientError()
    rootDirectory = lib61850.IedConnection_getFileDirectory(
        con, error_file, iec_dir)
    if error_file.value != lib61850.IED_ERROR_OK:
        print("Error retrieving file directory")

    else:
        directoryEntry = lib61850.LinkedList_getNext(rootDirectory)

        while directoryEntry:

            entry = ctypes.cast(lib61850.LinkedList_getData(
                directoryEntry), lib61850.FileDirectoryEntry)

            file_name = lib61850.FileDirectoryEntry_getFileName(
                entry).decode("UTF-8")
            file_size = lib61850.FileDirectoryEntry_getFileSize(entry)
            file_modified = lib61850.FileDirectoryEntry_getLastModified(
                entry)
            print("name: %s size: %d modified: %s" %
             (file_name, file_size, file_modified))
            for v in extension_file_dr:
                if v in file_name:
                    listFile.append([file_name, file_modified])

            directoryEntry = lib61850.LinkedList_getNext(directoryEntry)
        lib61850.LinkedList_destroy(directoryEntry)
        return listFile
    # else:
    #    print("connection issue")


def ClientFile61850_get(con, fileNameAsal):
    # update file latest
    #rootDir = "/home/pi/dms/DMSv1.2/IEC61850/DR_FILES"
    rootDir = "/var/www/html/dms_setting/assets/api/file_dr/"
    dirName = fileNameAsal[:-4]
    dirName = rootDir+dirName
    if not os.path.exists(dirName):
        os.mkdir(dirName)
    # update file latest

    handler = lib61850.IedClientGetFileHandler(_getFileHandler)
    if 'COMTRADE/' in fileNameAsal:
        localfilename = fileNameAsal.replace('COMTRADE/', '')
    elif '\\' in fileNameAsal:
        localfilename = fileNameAsal.replace('\\', '')

    open(localfilename, "w").close()  # create an empty file
    error_getfile = lib61850.IedClientError()

    # update file latest
    newfile = dirName+"\\"+localfilename
    fileNameOutput = bytes(newfile.encode('utf-8'))
    #print(fileNameOutput)
    # update file latest

    for i in range(max_client):
        lib61850.IedConnection_getFile(
            con[i]['con'], error_getfile, fileNameAsal, handler, ctypes.c_char_p(fileNameOutput))
        if error_getfile.value != lib61850.IED_ERROR_OK:
            print("Failed to get file!")


def Client61850_config():

    if DEBUG:
        for i in range(0, max_client):
            hostname[i] = dataDevice[i]
            tcpport[i] = 102
            error[i]['error'] = lib61850.IedClientError()
            con[i]['con'] = lib61850.IedConnection_create()

        for i in range(max_client):
            lib61850.IedConnection_connect(
                con[i]['con'], ctypes.byref(error[i]['error']), hostname[i], int(tcpport[i]))
    else:
        for i in range(0, max_client):
            hostname.append(
                {'id': dataDevice[i]["id_device"],
                 'hostname': dataDevice[i]["ip_address"]})
            # ipserver[i] = DBdataMeasurement[i]['ipserver']
            tcpport.append(
                {'id': dataDevice[i]["id_device"],
                 'port': dataDevice[i]["port_address"]})
            error.append(
                {'id': dataDevice[i]["id_device"],
                 'error': lib61850.IedClientError()})
            con.append(
                {'id': dataDevice[i]["id_device"],
                 'con': lib61850.IedConnection_create()})

            object_read.append(
                {'id': dataDevice[i]["id_device"]})

            object_FC.append(
                {'id': dataDevice[i]["id_device"]})

            alias_monit.append(
                {'id': dataDevice[i]["id_device"]})

            alias_name.append(
                {'id': dataDevice[i]["id_device"]})

            rack_location.append(
                {'id': dataDevice[i]["rack_location"]})

            type_relay.append(
                {'id': dataDevice[i]["type"]})
                
            iecfolder.append(
                {'id': dataDevice[i]["iec_file"]})
                
            disturbanceType.append(
                {'type': dataDevice[i]["disturbance_type"]})
            #print(disturbanceType)

    for i in range(0, max_client):
        lib61850.IedConnection_connect(
            con[i]['con'], ctypes.byref(error[i]['error']), hostname[i]['hostname'], int(tcpport[i]['port']))
        if (error[i]['error'].value == lib61850.IED_ERROR_OK):
            print("IEC61850 Connection OK in %s:%s" % (
                hostname[i]['hostname'], tcpport[i]['port']))
        else:
            print("IEC61850 Connection error in %s:%s" % (
                hostname[i]['hostname'], tcpport[i]['port']))


def CLient61850_itemConfig():
    # for k,v in DBdataMeasurement.items():
    #     if v in object_read['id']
    for v in DBdataMeasurement:
        # if v in object_read['id']
        if v["active"] == 1:
            for i in range(len(object_read)):
                if v['id_device'] == alias_monit[i]['id']:
                    if 'alias' in alias_monit[i]:
                        print('key obj')
                    else:
                        alias_monit[i]['alias'] = []
                        alias_name[i]['name'] = []

                    alias_monit[i]['alias'].append(v['alias'])
                    alias_name[i]['name'].append(v['name'])

                if v['id_device'] == object_read[i]['id']:
                    # print(i)
                    # if 'alias' in alias_monit[i] :
                    #     print('key obj')
                    # else:
                    #     alias_monit[i]['alias'] = []

                    # alias_monit[i]['alias'].append({'id': dataDevice[i]["id_device"],'obj': v['alias']})

                    fc_split = v["item_id"].split("$")
                    item_id = v["item_id"].replace("$"+fc_split[1], "")
                    #print(item_id)
                    # object_FC[i].append(_FC(fc_split[1]))
                    if 'obj' in object_read[i] or 'obj' in object_FC[i]:
                        print('key obj')
                    else:
                        object_read[i]['obj'] = []
                        object_read[i]['itemID'] = []
                        object_FC[i]['obj'] = []

                    object_read[i]['obj'].append(
                        v["domain_id"]+"/"+item_id.replace("$", "."))
                    object_read[i]['itemID'].append(
                        v["domain_id"]+"/"+item_id)
                    object_FC[i]['obj'].append(_FC(fc_split[1]))
                    # j += 1


def CLient61850_monitoringRead(con, hostname, error, object_read, object_fc):
    if (error.value == lib61850.IED_ERROR_OK):
        value = lib61850.IedConnection_readObject(con, ctypes.byref(
            error), object_read, object_fc)
        # print(object_read)
        if (error.value == lib61850.IED_ERROR_OK):
            val, types = iec61850client.printValue(value)
            return val, types
        return 0, 0
    else:
        print("Failed to connect to %s:%s" % (hostname, "102"))
        #Client61850_checkConnect(hostname, 102, con, error)
        return 0, 0


def CLient61850_monitoringLoop():
    for i in range(0, max_client):
        for j in range(len(object_read[i])):
            value = lib61850.IedConnection_readObject(con[0], ctypes.byref(
                error[0]), object_read[i][j], object_FC[i][j])
            val, types = iec61850client.printValue(value)
            #print(val, types)
            # if types == 'utc-time':


def CLient61850_destroyConn():
    for i in range(0, max_client):
        lib61850.IedConnection_release(con[i]['con'], error[i]['error'])
        if (error[i]['error'].value != lib61850.IED_ERROR_OK):
            print("Release returned error: %d" % error[i]['error'].value)
        else:
            while (lib61850.IedConnection_getState(con[i]['con']) != lib61850.IED_STATE_CLOSED):
                lib61850.Thread_sleep(10)

        lib61850.IedConnection_destroy(con[i]['con'])


def isOpen(ip, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((ip, int(port)))
        s.shutdown(socket.SHUT_RDWR)
        print("IP: %s Port: %d Opened" % (ip, port))
        return True
    except:
        print("IP: %s Port: %d Closed" % (ip, port))
        return False
    finally:
        s.close()


def Client61850_checkConnect(host, port, con, error):
    # response = os.system("ping -n 1 " + '192.168.2.13')
    response = isOpen(host, port)
    #if not response:
#    lib61850.IedConnection_connect(
#        con, ctypes.byref(error), host, int(port))
#    if (error.value == lib61850.IED_ERROR_OK):
#        print("IEC61850 Connection OK in %s:%s" % (
#            host, port))
#    else:
#        print("IEC61850 Connection error in %s:%s" % (
#            host, port))
#    else:
#        if (error.value == lib61850.IED_ERROR_OK):
#            print("IEC61850 Connection OK in %s:%s" % (
#                host, port))
#        else:
#            print("IEC61850 Connection error in %s:%s and Reconnecting" % (
#                host, port))
#            lib61850.IedConnection_connect(
#                con, ctypes.byref(error), host, int(port))


def connectMqtt(mqtt):
    global mqttRetry
    global client
    if (client != None):
        print("rekonnect mqtt")
        client.disconnect()
    dbMqtt = db.readDb.network_mqtt(0)
    mqttUser = dbMqtt[0]['mqtt_username']
    mqttPass = dbMqtt[0]['mqtt_pass']
    broker = dbMqtt[0]['mqtt_server']
    port = dbMqtt[0]['mqtt_port']
    try:
        client = mqtt.connect_mqtt(mqttUser, mqttPass, broker, port)
        client.loop_start()
        # client.loop_forever()
    except:
        print("mqttNOtconnect")
        if (mqttRetry < 10):
            connectMqtt(mqtt)


def ClientFile61850_getFile(con, fileNameAsal,directory):
    # create folder
    dirName = ''
    localfilename = ''
    #rootDir = "/home/pi/dms/DMSv1.2/IEC61850/DR_FILES"
    rootDir = "/var/www/html/dms_setting/assets/api/file_dr/"
#    if "COMTRADE" in fileNameAsal:
#        dirName = fileNameAsal.replace("COMTRADE", "")
#        print(dirName)
#        localfilename = dirName
#        dirName = dirName[:-4]
#    else:
#        dirName = fileNameAsal[:-4]
#        localfilename = fileNameAsal
#    dirName = rootDir+dirName

    if "\\" in fileNameAsal:
        buff = fileNameAsal.split("\\")
        dirName = buff[-1]
        localfilename = dirName
        dirName = dirName[:-4]
    elif "/" in fileNameAsal:
        buff = fileNameAsal.split("/")
        dirName = buff[-1]
        localfilename = dirName
        dirName = dirName[:-4]
    else:
        dirName = fileNameAsal[:-4]
        localfilename = fileNameAsal
    dirName = rootDir+"/"+dirName
    
    #print(dirName)
    if not os.path.exists(dirName):
        os.mkdir(dirName)

    handler = lib61850.IedClientGetFileHandler(_getFileHandler)
    # localfilename = fileNameAsal.replace('\\', '')
    localfilename = localfilename.replace('/', '')
    open(dirName+"/"+localfilename, "w").close()  # create an empty file
    # print(dirName+"\\"+localfilename)
    error_getfile = lib61850.IedClientError()

    newfile = dirName+"/"+localfilename
    fileNameOutput = bytes(newfile.encode('utf-8'))
    # print(fileNameOutput)

    lib61850.IedConnection_getFile(
        con, error_getfile, directory+fileNameAsal, handler, ctypes.c_char_p(fileNameOutput))
    if error_getfile.value != lib61850.IED_ERROR_OK:
        print("Failed to get file!")


def newFileMechanism(conn, fileList,directory):
    # sorting
    listFileNew = sorted(
        fileList, key=lambda fileList: fileList[1], reverse=True)
    # listFileNew = listfile
    FileFinal = listFileNew[0][0]
    for e in extension_file_dr:
        if e in FileFinal:
            #print(e)
            FileFinal = FileFinal.replace(e, "")
    #print(FileFinal)

    ExtensionFinal = []
    for v in listFileNew:
        for e in extension_file_dr:
            if FileFinal+e in v[0]:
                ExtensionFinal.append(e)
    #print(ExtensionFinal)
    for e in ExtensionFinal:
        ClientFile61850_getFile(conn, FileFinal+e,directory)
    return FileFinal, ExtensionFinal


def buildZip(dirName):
#    if "COMTRADE" in dirName:
#        dirName = dirName.replace("COMTRADE", "")
#        print(dirName)
    
    if "\\" in dirName:
        buff = dirName.split("\\")
        dirName = buff[-1]
    elif "/" in dirName:
        buff = dirName.split("/")
        dirName = buff[-1]
    dirName = "/"+dirName
    
    #rootDir = '/home/pi/dms/DMSv1.2/IEC61850/DR_FILES'
    rootDir = "/var/www/html/dms_setting/assets/api/file_dr/"
    filesDir = rootDir+dirName
    with ZipFile(filesDir + '.zip', 'w') as zipObj:
        for folderName, subfolders, filenames in os.walk(filesDir):
            for filename in filenames:
                absname = os.path.abspath(os.path.join(filesDir, filename))
                arcname = absname[len(rootDir) + 1:]
                print('zipping %s as %s' % (os.path.join(filesDir, filename),
                                            arcname))
                zipObj.write(absname, arcname,
                             compress_type=zipfile.ZIP_DEFLATED)
        zipObj.close()
    return filesDir+".zip"


def paramStatusTelegram(relayId, rack, alias, value):
    now = datetime.now()
    waktus = now.strftime("%d/%m/%Y %H:%M:%S")
    d = {"machineCode": mesin_mesin, "relayId": relayId, "lokasi": mesin_namaGi+" "+rack, "status": alias + " " + str(value),
         "namaFile": "STRR" + ' Disturbance.000', "waktu": waktus, "flag": str(0), "type": "IEC61850",
         "portType": 1, "portNumber": 0}

    #print(d)
    # response = requests.post("http://"+str(dbCfg3[0])+"/dms/fail/add", data=d)
    # print(response)
    try:
        r = requests.post(
            "http://"+network_ipserver+"/mel/dms/fail/add_event", data=d)
        r.raise_for_status()
    except requests.exceptions.HTTPError as errh:
        print("Http Error:", errh)
    except requests.exceptions.ConnectionError as errc:
        print("Error Connecting:", errc)
    except requests.exceptions.Timeout as errt:
        print("Timeout Error:", errt)
    except requests.exceptions.RequestException as err:
        print("OOps: Something Else", err)


def DFRFailADD(relayId, rack, DFRPath, types):
    now = datetime.now()
    waktus = now.strftime("%d/%m/%Y %H:%M:%S")

    url = 'http://'+network_ipserver+'/mel/dms/fail/add'
    directory = "/var/www/html/dms_setting/assets/api/file_dr/"
    DFRName = DFRPath.replace(
        directory, '')
    DFRName = DFRName.replace(".zip", "")
    
    comtradeFolder = DFRPath.replace(".zip", "")
    ret_comtrade = dfr_parse.getDFRvalue(comtradeFolder)
    
    #print(DFRPath)
    d = {"machineCode": mesin_mesin,
         "relayId": relayId,
         "lokasi": mesin_namaGi+" "+rack,
         "status": "DFR Available",
         "namaFile": DFRName,
         "waktu": waktus,
         "flag": 0,
         "type": types,
         "portType": "IEC61850",
         "portNumber": 0,
         "dr_metering":ret_comtrade[0],
         "dr_event":ret_comtrade[1]
         }

    #print(d)

    try:
        r = requests.post(
            url, data=d)
        r.raise_for_status()
    except requests.exceptions.HTTPError as errh:
        print("Http Error:", errh)
    except requests.exceptions.ConnectionError as errc:
        print("Error Connecting:", errc)
    except requests.exceptions.Timeout as errt:
        print("Timeout Error:", errt)
    except requests.exceptions.RequestException as err:
        print("OOps: Something Else", err)


def DFRUploadFile(relayId, rack, DFRPath, types):

    now = datetime.now()
    waktus = now.strftime("%d/%m/%Y %H:%M:%S")

    url = 'http://'+network_ipserver+'/mel/dms/notif'

    fileZip = {'myfile': open(DFRPath, 'rb')}
    #print(DFRPath)
    directory = "/var/www/html/dms_setting/assets/api/file_dr/"
    DFRName = DFRPath.replace(
        directory, '')
    DFRName = DFRName.replace(".zip", "")
    #print(DFRName)
    d = {"status": "DFR Available",
         "file_name": DFRName,
         "machineCode": mesin_mesin,
         "lokasi": mesin_namaGi+" "+rack,
         "namaFile": DFRName,
         "waktu": waktus,
         "flag": 0,
         "type": types,
         "portType": "IEC61850",
         "relayId": relayId,
         "portNumber": 0
         }

    #print(d)
    try:
        r = requests.post(url,  data=d, files=fileZip, verify=False)
        r.raise_for_status()
        if r.status_code != 200:
            print('sendErr: '+r.url)
            db.insertDb.m_fileDR_temp(2,relayId,str(relayId),str(d),str(0),DFRName)
        else:
            print(r.text)
            db.insertDb.m_fileDR_temp(2,relayId,str(relayId),str(d),str(1),DFRName)
    except requests.exceptions.HTTPError as errh:
        print("Http Error:", errh)
        db.insertDb.m_fileDR_temp(0,'2',str(relayId),str(d),str(0),DFRName)
    except requests.exceptions.ConnectionError as errc:
        print("Error Connecting:", errc)
        db.insertDb.m_fileDR_temp(0,'2',str(relayId),str(d),str(0),DFRName)
    except requests.exceptions.Timeout as errt:
        print("Timeout Error:", errt)
        db.insertDb.m_fileDR_temp(0,'2',str(relayId),str(d),str(0),DFRName)
    except requests.exceptions.RequestException as err:
        print("OOps: Something Else", err)
        db.insertDb.m_fileDR_temp(0,'2',str(relayId),str(d),str(0),DFRName)
        
#    r = requests.post(url,  data=d, files=fileZip, verify=False)
#    if r.status_code != 200:
#        print('sendErr: '+r.url)
#        db.insertDb.m_fileDR_temp(0,'2',str(relayId),str(d),str(0),nameFile)
#    else:
#        print(r.text)
#        db.insertDb.m_fileDR_temp(x,'2',str(relayId),str(d),str(1),nameFile)

#mqtt = mqtt.MQTT(0)
#connectMqtt(mqtt)


#print(network_ipserver)

Client61850_config()
CLient61850_itemConfig()
flagNotif = 0


# update file berdasarkan latest
listfileArr = []
for i in range(0, max_client):
    if disturbanceType[i]['type'] == '1':
        listfileArr.append(ClientFile61850_dir(con[i]['con'],iecfolder[i]['id']))
    else:
        listfileArr.append(None)
    # listfileArr[i].sort(key=lambda x: x[1], reverse=False)
    # for j in range(len(listfileArr[i])):
    #     print(listfileArr[i][j])
listfileArrPrev = []
flags = 0
#print(len(listfileArr))
# update file berdasarkan latest

# update status notif telegram
valueDataBoolean = []
for i in range(0, max_client):
    valueDataBoolean.append([])
    if 'obj' in object_read[i]:
        for j in range(len(object_read[i]['obj'])):
            value, types = CLient61850_monitoringRead(
                con[i]['con'], hostname[i]['hostname'], error[i]['error'], object_read[i]['obj'][j], object_FC[i]['obj'][j])
            valueDataBoolean[i].append(value)
valueDataBooleanPrev = valueDataBoolean
flagStatus = 0
# update status notif telegram

# print(object_FC)
listFIleprev = 0

try:
    while True:
        lib61850.Thread_sleep(200)
        for i in range(0, max_client):

            # update file latest
            if disturbanceType[i]['type'] == '1':
                if flags == 0:
                    listfileArrPrev = listfileArr
                    flags = 1
                #print(ClientFile61850_dir(con[i]['con'],iecfolder[i]['id']))
                #print(listfileArr[i])
                if ClientFile61850_dir(con[i]['con'],iecfolder[i]['id']) != listfileArr[i]:
                    print("DATA BARU")
                    listfileArr[i] = ClientFile61850_dir(con[i]['con'],iecfolder[i]['id'])
                    dirName, extFile = newFileMechanism(
                        con[i]['con'], listfileArr[i],iecfolder[i]['id'])
                    dr_file = buildZip(dirName)
                    print("UPLOAD FILE", dirName)
                    # send api failadd then notif
                    DFRFailADD(con[i]['id'], rack_location[i]['id'],
                               dr_file, type_relay[i]['id'])
                    DFRUploadFile(
                        con[i]['id'], rack_location[i]['id'], dr_file, type_relay[i]['id'])

            # Client61850_checkConnect(hostname[i]['hostname'], int(tcpport[i]['port']), con[i]['con'], error[i]['error'])
            if 'obj' in object_read[i]:
                for j in range(len(object_read[i]['obj'])):
                    #print("counting object %d" % (j))
                    #try:
                    value, types = CLient61850_monitoringRead(
                        con[i]['con'], hostname[i]['hostname'],error[i]['error'], object_read[i]['obj'][j], object_FC[i]['obj'][j])
                    dataJson = {
                        "alias": alias_monit[i]['alias'][j], "val": value, "dataType": types}
                    topic = f"DMS/{mesin_mesin}/IEC61850/{object_read[i]['id']}/{object_read[i]['itemID'][j]}/"
                    dataJson = str(dataJson)
                    dataJson = dataJson.replace("'", "\"")
                    #print(type(types))
                    if types == 0 or types == None:
                        print("Data is None")
                         
                    else:
                        try:
                             #mqtt.send(client, topic, str(dataJson))
                             mqtt_publish.single(topic, dataJson, hostname=dbMqtt[0]['mqtt_server'],
                                   port=dbMqtt[0]['mqtt_port'], auth={'username':dbMqtt[0]['mqtt_username'], 'password':dbMqtt[0]['mqtt_pass']})
                             mqtt_publish.single(topic, dataJson, hostname='localhost',port=1883)
                        except:
                             print("mqtt not connect")
                       
                    #except:
                    #   print("cannot get Data from client")

#         #time.sleep(0.5)

except KeyboardInterrupt:
    CLient61850_destroyConn()
    print('interrupted!')

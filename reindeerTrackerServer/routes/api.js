var express = require('express');
var ObjectId = require('mongoose').Types.ObjectId;
var router = express.Router();
var config = require('../config');

// IMPORTANT: LOGS ARE ADDED AT THE BEGINNING OF THE ARRAY

/////////////////////////////////////////////////////////////////
// DEVICES
/////////////////////////////////////////////////////////////////

var Device = require('../models/device');

router.post('/devices', function (req, res) {
    const deviceKeys = req.body;
    var keyErrors = [];
    var saved = [];
    for (var  i = 0; i < deviceKeys.length; i++) {
        const device = new Device({
            deviceKey:deviceKeys[i]
        });
        device.save()
            .then(function () {
                saved.push(device.deviceKey);
                checkAllDevicesSaved(deviceKeys, keyErrors, saved, res);
            }).catch(function (err) {
                keyErrors.push(device.deviceKey);
                checkAllDevicesSaved(deviceKeys, keyErrors, saved, res);
            })
    }
});

function checkAllDevicesSaved(deviceKeys, keyErrors, saved, res) {
    if (keyErrors.length + saved.length == deviceKeys.length) {
        if (keyErrors.length > 0) {
            res.status(500);
        } else {
            res.status(200);
        }
        res.json({
            added: saved,
            errors: keyErrors
        })
    }
}

router.post('/devices/single', function (req, res) {
    console.log('body: ' + req.body);
    const device = new Device({
        deviceKey:req.body
    });
    device.save()
        .then(function () {
            res.status(201).send('device added');
        })
        .catch(function (err) {
            res.status(500).send('could not add device')
        })
});

router.get('/devices/:deviceKey', function (req, res) {
    const deviceKey = req.params.deviceKey;
    Device.findOne({deviceKey: deviceKey}, {logs: { $slice: 5}})
        .then(function (device) {
            res.status(200).json(device);
        })
        .catch(function (err) {
            res.status(404).send('could not find device');
        })
});

router.get('/devices/:deviceKey/logs', function (req, res) {
    const deviceKey = req.params.deviceKey;
    var start = parseInt(req.query.start);
    const amount = parseInt(req.query.amount);
    if (isNaN(start)) {
        start = 0;
    }
    if (isNaN(amount)) {
        res.status(400).send('amount is not given, bad request');
    } else {
        Device.findOne({deviceKey: deviceKey}, {logs: { $slice: [start, amount]}})
            .then(function (device) {
                res.status(200).json(device.logs);
            })
            .catch(function (err) {
                res.status(404).send('could not find device');
            })
    }
});

router.put('/devices/:deviceKey/logs', function (req, res) {
    console.log('put');
    const deviceKey = req.params.deviceKey;
    const log = req.body;
    Device.update(
        {deviceKey: deviceKey},
        {$push: {logs: { $each: [log], $position: 0}}, isAlive: log.isAlive}
    )
        .then(function (status) {
            res.status(200).send('log added');
        })
        .catch(function (err) {
            res.status(500).send('could not add log to device');
        })
});

router.post('/devices/:deviceKey/details', function (req, res) {
    const deviceKey = req.params.deviceKey;
    const details = req.body;
    Device.update(
        {deviceKey: deviceKey},
        {name: details.name, birthDate: generateBirthDate(details.birthYear, details.birthMonth, details.birthDay), imageUrl: details.imageUrl, gender: details.gender}
    )
        .then(function (value) {
            if (value.n <= 0) {
                res.status(404).send('device not found')
            } else {
                res.status(200).json('data updated');
            }
        })
        .catch(function (reason) {
            res.status(500).send('could not add data');
        })
});

router.get('/devices/:deviceKey/details', function (req, res) {
    const deviceKey = req.params.deviceKey;
    Device.findOne({deviceKey: deviceKey})
        .then(function (device) {
            res.json(selectDeviceInfo([device], ['name', 'birthDate', 'imageUrl', 'gender']));
        })
        .catch(function (err) {
            res.status(404).send('Could not find device');
        })
});

router.put('/devices/:deviceKey/details', function (req, res) {
    const deviceKey = req.params.deviceKey;
    const details = req.body;
    Device.update(
        {deviceKey: deviceKey},
        {name: details.name, birthDate: generateBirthDate(details.birthYear, details.birthMonth, details.birthDay), imageUrl: details.imageUrl, gender: details.gender}
    )
        .then(function (value) {
            if (value.n <= 0) {
                res.status(404).send('device not found')
            } else {
                res.status(200).json('data updated');
            }
        })
        .catch(function (reason) {
            res.status(500).send('could not add data');
        })
});

router.put('/devices/:deviceKey/activate', function (req, res) {
    const deviceKey = req.params.deviceKey;
    Device.update(
        {deviceKey: deviceKey},
        {activated: true, activationDate: Date.now()}
    )
        .then(function (value) {
            if (value.n <= 0) {
                res.status(404).send('device not found')
            } else {
                res.status(200).json('data updated');
            }
        })
        .catch(function (reason) {
            res.status(500).send('could not add data');
        })
});

function generateBirthDate(year, month, day) {
    if (!month) {
        month = '01'
    }
    else if (month.length < 2) {
        month = '0' + month;
    }
    if (!day) {
        day = '01';
    }
    else if (day.length < 2) {
        day = '0' + day;
    }
    var dateString = year + '-' + month + '-' + day;
    console.log(dateString);
    return new Date(dateString);
}

/////////////////////////////////////////////////////////////////
// USERS
/////////////////////////////////////////////////////////////////

router.put('/users/:userID/devices', function (req, res) {
    const userID = req.params.userID;
    const deviceKey = req.body.deviceKey;
    Device.update(
        {deviceKey: deviceKey},
        {$push: {userIDs: userID}}
    )
        .then(function (value) {
            if (value.n <= 0) {
                res.status(404).send('device not found')
            } else {
                res.status(200).json('device registered');
            }
        })
        .catch(function (reason) {
            res.status(500).send('could not register device');
        });
});

router.get('/users/:userID/devices', function (req, res) {
    const userID = req.params.userID;
    Device.find({userIDs: userID})
        .then(function (devices) {
            //TODO: add other fields that need to be displayed in user list
            res.json(selectDeviceInfo(devices, ['deviceKey', "isAlive", 'lastLog', 'activated']))
        })
        .catch(function (err) {
            res.status(404).send('could not find devices');
        })
});

function selectDeviceInfo(devices, keys) {
    const returnDevices = [];
    for (var  i = 0; i < devices.length; i++) {
        var newObject = {};
        for (var  j = 0; j < keys.length; j++) {
            newObject[keys[j]] = devices[i][keys[j]];
        }
        returnDevices.push(newObject);
    }
    return returnDevices;
}

module.exports = router;
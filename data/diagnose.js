var endpoint = window.location.hostname;

document.getElementById('wings-out-button').addEventListener('click', handleWingsOut);
document.getElementById('wings-in-button').addEventListener('click', handleWingsIn);
document.getElementById('rotate-left-button').addEventListener('click', handleRotateLeft);
document.getElementById('rotate-right-button').addEventListener('click', handleRotateRight);
document.getElementById('gun-button').addEventListener('click', handleGun);
document.getElementById('led-ring-button').addEventListener('click', handleLEDRing);
document.getElementById('audio-button').addEventListener('click', handleAudio);

function handleWingsOut() {
    diagnose(0);
}

function handleWingsIn() {
    diagnose(1);
}

function handleRotateLeft() {
    diagnose(2);
}

function handleRotateRight() {
    diagnose(3);
}

function handleGun() {
    diagnose(4);
}

function handleLEDRing() {
    diagnose(5);
}

function handleAudio() {
    diagnose(6);
}

function diagnose(state) {
    var xhr = new XMLHttpRequest();
    var fd = new FormData();
    fd.append("action", state);
    xhr.open("POST", `http://${endpoint}/diagnose`, true);
    xhr.send(fd);
}
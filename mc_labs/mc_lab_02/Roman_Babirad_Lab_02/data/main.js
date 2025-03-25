const button1 = document.getElementById("algo1");
const button2 = document.getElementById("algo2");

let timer1, wasHeldEnough1, timer2, wasHeldEnough2;

button1.addEventListener('mousedown', startHold1);
button1.addEventListener('mouseup', stopHold1);
button1.addEventListener('touchstart', startHold1);
button1.addEventListener('touchend', stopHold1);

button2.addEventListener('mousedown', startHold2);
button2.addEventListener('mouseup', stopHold2);
button2.addEventListener('touchstart', startHold2);
button2.addEventListener('touchend', stopHold2);

function startHold1() {
  wasHeldEnough1 = false;
  timer1 = setTimeout(() => {
    wasHeldEnough1 = true;
    fetch('/hold').then();
  }, 500);
}

function stopHold1() {
  if (wasHeldEnough1) {
    fetch('/release').then();
  }
  clearTimeout(timer1);
  wasHeldEnough1 = false;
}

function startHold2() {
  wasHeldEnough2 = false;
  timer2 = setTimeout(() => {
    wasHeldEnough2 = true;
    fetch('/startAlgo2').then();
  }, 500);
}

function stopHold2() {
  if (wasHeldEnough2) {
    fetch('/stopAlgo2').then();
  }
  clearTimeout(timer2);
  wasHeldEnough2 = false;
}

const socket = new WebSocket("ws://172.20.10.2/ws");

socket.onmessage = (event) => {
  let data = event.data;
  document.querySelectorAll('.led').forEach(led => led.classList.remove('active'));

  if (data === "red") document.getElementById("red").classList.add("active");
  if (data === "yellow") document.getElementById("yellow").classList.add("active");
  if (data === "green") document.getElementById("green").classList.add("active");
};

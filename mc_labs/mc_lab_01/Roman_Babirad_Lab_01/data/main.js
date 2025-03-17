let timer, wasHeldEnough;

function startHold() {
  wasHeldEnough = false;
  timer = setTimeout(() => {
    wasHeldEnough = true;
    fetch('/hold').then();
  }, 1000);
}

function stopHold() {
  if (wasHeldEnough) {
    fetch('/release').then();
  }
  clearTimeout(timer);
  wasHeldEnough = false;
}
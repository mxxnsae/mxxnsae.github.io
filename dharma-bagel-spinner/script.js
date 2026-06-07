// Bagel configurations
const bagels = [
  { name: 'Plain', color: 0xD2B48C, roughness: 0.5, metalness: 0.1 },
  { name: 'Cream Cheese', color: 0xFFFAFA, roughness: 0.3, metalness: 0.15 },
  { name: 'Smoked Salmon', color: 0xCD853F, roughness: 0.6, metalness: 0.05 },
  { name: 'Blueberry', color: 0xC9B8A8, roughness: 0.7, metalness: 0.0 },
  { name: 'Sesame', color: 0xF4A460, roughness: 0.8, metalness: 0.0 },
  { name: 'Everything', color: 0x1a1a1a, roughness: 0.5, metalness: 0.1 },
  { name: 'UFO', color: 0x00E5CC, roughness: 0.3, metalness: 0.8 },
];

const bgColors = [
  [0.98, 0.98, 0.98],
  [0.97, 0.97, 0.99],
  [0.99, 0.98, 0.97],
  [0.96, 0.98, 0.99],
  [0.98, 0.97, 0.98],
  [0.97, 0.99, 0.98],
  [0.95, 0.96, 1.0],
];

let currentIndex = 0;
let isRotating = false;

// Scene setup
const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(
  75,
  window.innerWidth / window.innerHeight,
  0.1,
  1000
);
camera.position.z = 6;

const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.shadowMap.enabled = true;
document.getElementById('canvas-container').appendChild(renderer.domElement);

// Lighting
const ambientLight = new THREE.AmbientLight(0xffffff, 0.6);
scene.add(ambientLight);

const directionalLight = new THREE.DirectionalLight(0xffffff, 0.8);
directionalLight.position.set(5, 5, 5);
directionalLight.castShadow = true;
scene.add(directionalLight);

const pointLight = new THREE.PointLight(0xff00ff, 0.3);
pointLight.position.set(-5, 5, 3);
scene.add(pointLight);

// Create bagel group (UFO and regular bagels will use different geometries)
let bagel;

function createBagel(bagelData, isUFO = false) {
  if (bagel) scene.remove(bagel);

  if (isUFO) {
    // UFO model (simple)
    bagel = new THREE.Group();

    // Donut base
    const donutGeometry = new THREE.TorusGeometry(2, 0.8, 16, 100);
    const donutMaterial = new THREE.MeshStandardMaterial({
      color: bagelData.color,
      roughness: bagelData.roughness,
      metalness: bagelData.metalness,
      emissive: 0x00FFFF,
      emissiveIntensity: 0.15,
    });
    const donut = new THREE.Mesh(donutGeometry, donutMaterial);
    donut.castShadow = true;
    donut.receiveShadow = true;
    bagel.add(donut);

    // Dome (sphere on center - same axis as donut)
    const domeGeometry = new THREE.SphereGeometry(1.3, 32, 32);
    const domeMaterial = new THREE.MeshStandardMaterial({
      color: 0xFFFFFF,
      roughness: 0.2,
      metalness: 0.7,
      transparent: true,
      opacity: 0.65,
      emissive: 0x00FF88,
      emissiveIntensity: 0.15,
    });
    const dome = new THREE.Mesh(domeGeometry, domeMaterial);
    dome.position.set(0, 0, 0); // Same center as donut
    dome.castShadow = true;
    dome.receiveShadow = true;
    bagel.add(dome);
  } else {
    // Regular bagel (donut)
    const geometry = new THREE.TorusGeometry(2, 0.8, 16, 100);
    const material = new THREE.MeshStandardMaterial({
      color: bagelData.color,
      roughness: bagelData.roughness,
      metalness: bagelData.metalness,
    });
    bagel = new THREE.Mesh(geometry, material);
    bagel.castShadow = true;
    bagel.receiveShadow = true;
  }

  scene.add(bagel);
}

// Initialize first bagel
createBagel(bagels[0], false);

// Set random initial rotation
bagel.rotation.y = Math.random() * Math.PI * 2;
bagel.rotation.x = Math.random() * Math.PI * 2;

// Set background
scene.background = new THREE.Color(...bgColors[0]);

// Animation loop
function animate() {
  requestAnimationFrame(animate);

  // Smooth rotation
  bagel.rotation.y += 0.01;
  bagel.rotation.x += 0.003;

  renderer.render(scene, camera);
}

// Handle click
document.addEventListener('click', () => {
  if (isRotating) return;

  isRotating = true;

  // Change bagel
  currentIndex = (currentIndex + 1) % bagels.length;
  const nextBagel = bagels[currentIndex];
  const isNextUFO = currentIndex === 6; // UFO is at index 6

  // Recreate bagel with appropriate model
  createBagel(nextBagel, isNextUFO);

  // Animate bagel scale for spin effect
  bagel.scale.set(1, 1, 1);
  const startScale = 1;
  const endScale = 1.1;
  const duration = 300;
  let elapsed = 0;

  const scaleInterval = setInterval(() => {
    elapsed += 16;
    const progress = Math.min(elapsed / duration, 1);
    const easeProgress = progress < 0.5 ? 2 * progress * progress : -1 + (4 - 2 * progress) * progress;
    const scale = startScale + (endScale - startScale) * easeProgress;
    bagel.scale.set(scale, scale, scale);

    if (progress === 1) {
      clearInterval(scaleInterval);
      bagel.scale.set(1, 1, 1);
      isRotating = false;
    }
  }, 16);

  // Animate background change
  const startBg = new THREE.Color(...bgColors[(currentIndex - 1 + bagels.length) % bagels.length]);
  const endBg = new THREE.Color(...bgColors[currentIndex]);
  elapsed = 0;

  const bgInterval = setInterval(() => {
    elapsed += 16;
    const progress = Math.min(elapsed / duration, 1);

    const currentBg = new THREE.Color().lerpColors(startBg, endBg, progress);
    scene.background = currentBg;

    if (progress === 1) {
      clearInterval(bgInterval);
    }
  }, 16);

  // Update UI
  document.getElementById('bagelName').textContent = nextBagel.name;
});

// Handle window resize
window.addEventListener('resize', () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
});

// Start animation
animate();

// Set initial UI
document.getElementById('bagelName').textContent = bagels[0].name;

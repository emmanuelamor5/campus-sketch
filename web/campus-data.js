// Campus names and locations supplied by the campus owner.
export const views = {
  overview: { label: 'Overview', title: 'Keri Campus overview', pos: [145,130,175], target: [20,0,20] },
  entrance: { label: 'Main gate', title: 'Keri Campus · Main gate', pos: [-42,19,-76], target: [-17,3,-35.5] },
  admin: { label: 'Admin Offices', title: 'Admin Offices', pos: [-38,34,9], target: [0,4,-10] },
  lectures: { label: 'Lecture Rooms', title: 'Lecture Rooms (LT 1 – LT 6)', pos: [112,38,12], target: [60,4,-10] },
  chapel: { label: 'Chapel', title: 'Chapel', pos: [77,29,49], target: [44,4,23] },
  courtyard: { label: 'Courtyard gardens', title: 'Courtyard gardens', pos: [94,67,97], target: [35,0,35] },
  labs: { label: 'Labs & Rooms', title: 'Labs & Rooms', pos: [40,20,41], target: [6,3,36] },
  parking: { label: 'Parking', title: 'Parking', pos: [-67,40,53], target: [-23,0,20] },
  gazebos: { label: 'Gazebos', title: 'Gazebos', pos: [17,23,-61], target: [15,1,-27] },
  leisure: { label: 'Leisure Point', title: 'Leisure Point', pos: [73,27,58], target: [42,1,38] },
  plan: { label: 'Site plan', title: 'Keri Campus · Site plan', pos: [20,230,20.1], target: [20,0,20] }
};
export const places = [
  { name: 'Admin Offices', view: 'admin', point: [0,6,-10], text: 'The administrative offices are in the connected wing of the campus complex.' },
  { name: 'Lecture Rooms (LT 1 – LT 6)', view: 'lectures', point: [60,7,-10], text: 'Lecture rooms LT 1 through LT 6 occupy this teaching block.' },
  { name: 'Courtyard gardens', view: 'courtyard', point: [35,1.5,37], text: 'Green courtyards and connecting paths sit between the campus buildings.' },
  { name: 'Parking', view: 'parking', point: [-22,1.5,20], text: 'The parking area follows the tree-lined approach along the side of the campus.' },
  { name: 'Chapel', view: 'chapel', point: [44,6,23], text: 'The chapel is the building in the middle of the campus complex.' },
  { name: 'Gazebos', view: 'gazebos', point: [15,3,-27], text: 'The gazebos are beside the garden near the relocated main gate: a sheltered place to sit and meet.' },
  { name: 'Leisure Point', view: 'leisure', point: [45,1.5,39], text: 'Take a break among the courtyard lawns, flowering borders and shaded seating.' },
  { name: 'Labs & Rooms', view: 'labs', point: [6,6,37], text: 'The building to the left of the highlighted courtyard contains the labs and rooms.' }
];
export const tourStops = ['entrance','parking','admin','lectures','chapel','courtyard','labs','gazebos','leisure','overview'];

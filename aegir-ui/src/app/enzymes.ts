export { EnzymeList, activeEnzymes }

export interface Enzyme {
  min: number,
  max: number,
  name: string,
  target: string,
  info: string,
  duration?: number,
};

let EnzymeList: Enzyme[] = [
  { min: 40,
    max: 45,
    name: "β-Glucanase",
    target: "β-Glucan",
    info: "Break down cell walls, extract efficiency"},
  { min: 50,
    max: 54,
    name: "Protease",
    target: "Protein",
    info: "Head, haze reduction"},
  { min: 55,
    max: 65,
    name: "β-Amylase",
    target: "Starch",
    info: "maltose"},
  { min: 63,
    max: 70,
    name: "α-Amylase",
    target: "Starch",
    info: "Complex maltose"},
];

function activeEnzymes(temp: number): Enzyme[] {
  let active: Enzyme[] = [];

  Object.entries(EnzymeList)
    .forEach(
      ([key, data]) => {
	if ( data.min <= temp && data.max >= temp ) {
	  active.push(data);
	}
      }
    );
  return active;
}

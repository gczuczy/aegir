export { sg2brix, brix2sg };

function sg2brix(sg: number): number {
  return 182.4601*(sg**3) - 775.6821*(sg ** 2) + 1262.7794*sg - 669.5622;
}

function brix2sg(brix: number): number {
  return (brix/(258.6-((brix/258.2)*227.1)))+1
}

export interface apiStateTemps {
  BK: number;
  HLT: number;
  MashTun: number;
  RIMS: number;
};

export interface apiStateData {
  levelerror: boolean;
  state: string;
  targettemp: number;
  currtemp: apiStateTemps;
};

export interface apiStateResponse {
  status: string;
  data?: apiStateData;
};

export interface apiConfig {
  status: string,
  hepower: string,
  tempaccuracy: number,
  cooltemp: number,
  heatoverhead: number,
  hedelay: number
};

export interface apiConfigResponse {
  data: apiConfig;
  status: string
}

export interface apiProgramMashStep {
  id?: number,
  orderno: number,
  temperature: number,
  holdtime: number
}

export interface apiProgramHopStep {
  id?: number,
  attime: number,
  hopname: string,
  hopqty: number
}

export interface apiProgram {
  id?: number,
  starttemp: number,
  endtemp: number,
  boiltime: number,
  name: string,
  nomash: boolean,
  noboil: boolean,
  mashsteps: apiProgramMashStep[],
  hops: apiProgramHopStep[]
}

export interface apiProgramsResponse {
  data: apiProgram[],
  status: string
}

export interface apiProgramResponse {
  data: apiProgram,
  status: string
}

export interface apiProgramDeleteResponse {
  status: string,
  errors: string[]
}

export interface apiAddProgramData {
  progid: number,
}

export interface apiAddProgramResponse {
  status: string,
  data: apiAddProgramData,
  errors: string[]
}

export interface apiSaveProgramData {
  progid: number,
}

export interface apiSaveProgramResponse {
  status: string,
  data: apiSaveProgramData,
  errors: string[]
}

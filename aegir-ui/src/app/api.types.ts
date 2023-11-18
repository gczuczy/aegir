export interface apiStateTemps {
  BK: number;
  HLT: number;
  MT: number;
  RIMS: number;
};

export interface apiStateMashStep {
  time: number,
  orderno: number,
  textual?: string,
};

export interface apiStateCooling {
  ready: boolean,
  cooltemp: number
};

export interface hoppingSchedule {
  id: number,
  name: string,
  qty: number,
  done: boolean,
  tth: string,
};

export interface apiStateHopping {
  hoptime: number,
  schedule?: hoppingSchedule[],
};

export interface apiStateData {
  levelerror: boolean,
  state: string,
  targettemp: number,
  currtemp: apiStateTemps,
  programid?: number,
  mashstep?: apiStateMashStep,
  cooling?: apiStateCooling,
  hopping?: apiStateHopping,
};

export interface apiStateResponse {
  status: string;
  data?: apiStateData;
};

export interface apiConfig {
  status?: string,
  hepower: string,
  tempaccuracy: number,
  cooltemp: number,
  heatoverhead: number,
  hedelay: number,
  loglevel: string,
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

export interface apiBrewStateVolumeData {
  volume: number
}

export interface apiBrewStateVolume {
  status: string,
  data: apiBrewStateVolumeData,
  errors: string[]
}

export interface apiBrewTempHistoryData {
  dt: number[],
  rims: number[],
  mt: number[],
  last: number,
}

export interface apiBrewTempHistoryResponse {
  status: string,
  data: apiBrewTempHistoryData,
  errors: string[]
}

export enum LoadProgramStartMode {
  Now = "now",
  Timed = "timed",
}

export interface apiBrewLoadProgramRequest {
  id: number,
  startat: string,
  startmode: LoadProgramStartMode,
  volume: number
}

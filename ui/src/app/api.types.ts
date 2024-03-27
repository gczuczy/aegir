
export interface apiResponse {
  status: string,
  data?: any,
  message?: string,
  errors?: string[],
};

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

export interface apiConfig {
  status?: string,
  hepower: string,
  tempaccuracy: number,
  cooltemp: number,
  heatoverhead: number,
  hedelay: number,
  loglevel: string,
};

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

export interface apiAddProgram {
  progid: number,
}

export interface apiSaveProgramData {
  progid: number,
}

export interface apiBrewStateVolumeData {
  volume: number
}

export interface apiBrewStateVolume {
  status: string,
  data: apiBrewStateVolumeData,
  errors: string[]
}

export interface apiBrewTempHistory {
  dt: number[],
  rims: number[],
  mt: number[],
  last: number,
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

export interface apiFermd {
  id?: number,
  name: string,
  address: string,
}

export interface apiFermenterType {
  id?: number,
  name: string,
  capacity: number,
  imageurl: string,
}

export interface apiFermenter {
  id?: number,
  name: string,
  type: apiFermenterType,
}

export interface apiTilthydrometer {
  id?: number,
  color: string,
  uuid: string,
  enabled: boolean,
  calibr_null?: number,
  calibr_at?: number,
  calibr_sg?: number,
  fermenter?: apiFermenter|string,
}

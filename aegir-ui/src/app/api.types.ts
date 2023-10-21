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

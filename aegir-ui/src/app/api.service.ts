import { Injectable } from '@angular/core';

import { HttpClient, HttpParams, HttpHeaders } from '@angular/common/http';

import { timer, Observable, BehaviorSubject } from 'rxjs';
import { map } from 'rxjs/operators';

import { apiStateResponse, apiStateData,
	 apiConfigResponse, apiConfig,
	 apiProgramsResponse, apiProgram,
	 apiProgramResponse, apiProgramDeleteResponse,
	 apiAddProgramResponse, apiAddProgramData,
	 apiSaveProgramResponse, apiSaveProgramData,
	 apiBrewStateVolume, apiBrewStateVolumeData,
	 apiBrewTempHistoryResponse, apiBrewTempHistoryData,
	 apiBrewLoadProgramRequest
       } from './api.types';

@Injectable({
  providedIn: 'root'
})
export class ApiService {

  public updateAnnounce$ = new BehaviorSubject<boolean>(false);

  // observable state
  private timer_state;
  private timer_state_sub;
  private timer_temphistory;
  private timer_temphistory_sub;
  private state_data = 'Empty';
  public temphistory$ = new BehaviorSubject<apiBrewTempHistoryData|null>(null);
  private temphistory_last:number = 0;
  private temphistory_data:apiBrewTempHistoryData|null = null;
  private state = new BehaviorSubject(<apiStateData>{
    levelerror: false,
    state: 'Empty',
    targettemp: 0,
    currtemp: {
      BK: 0,
      HLT: 0,
      MT: 0,
      RIMS: 0
    }
  });

  constructor(private http: HttpClient) {
    //console.log('ApiService ctor');
    this.timer_state = timer(1000, 1000);
    this.timer_state_sub = this.timer_state.subscribe((t:any) => {this.updateState(t)});
    this.timer_temphistory = timer(1000,5000);
    this.timer_temphistory_sub = this.timer_temphistory.subscribe(
      (t:any) => {
	this.updateTempHistory(t)
      }
    );
  }

  updateState(t:any) {
    this.http.get('/api/brewd/state')
      .pipe(
	map(res => <apiStateResponse>res)
      )
      .subscribe((res:apiStateResponse) => {
	//console.log('ApiService::updateState', res);
	if ( res.data == null ) return;

	let state = res.data.state
	this.state_data = res.data.state;
	let newstates = new Set(['Empty', 'Loaded', 'PreWait', 'PreHeat', 'NeedMalt']);
	if ( newstates.has(state) ) {
	  this.temphistory$.next(null);
	  this.temphistory_data = null;
	  this.temphistory_last = 0;
	}
	this.state.next(res.data);
      }, (err:any) => {
	console.log('updateState/err', err);
      });
  }

  getState(): BehaviorSubject<apiStateData> {
    return this.state;
  }

  updateTempHistory(t:any) {
    let newstates = new Set(['Maintenance', 'Empty', 'Loaded', 'PreWait', 'PreHeat', 'NeedMalt',
			     'PreBoil', 'Hopping', 'Cooling', 'Transfer', 'Finished']);
    if ( newstates.has(this.state_data) ) return;
    //console.log('state data', this.state_data);

    let params = new HttpParams().set('from', this.temphistory_last.toString());

    this.http.get(`/api/brewd/state/temphistory`, {'params': params})
      .pipe(
	map(res => (<apiBrewTempHistoryResponse>res).data)
      )
      .subscribe(
	(data:apiBrewTempHistoryData) => {
	  //console.log('temphistory result', data);
	  this.temphistory_last = data.last;

	  if ( this.temphistory_data == null ) {
	    this.temphistory_data = data;
	  } else {
	    // merge it to the temphistory_data
	    let last:number = this.temphistory_data!.dt[this.temphistory_data!.dt.length];
	    for (var i in data.dt) {
	      if ( data.dt[i] <= last ) continue;
	      this.temphistory_data.dt.push(data.dt[i]);
	      this.temphistory_data.rims.push(data.rims[i]);
	      this.temphistory_data.mt.push(data.mt[i]);
	    }
	  }
	  this.temphistory$.next(data);
	});
  }

  getAllTempHistory(): apiBrewTempHistoryData {
    return this.temphistory_data as apiBrewTempHistoryData;
  }

  startMaintenance(): Observable<any> {
    let body = JSON.stringify({'mode': 'start'});
    let headers = new HttpHeaders({'Content-type': 'application/json'});
    let options = {'headers': headers};

    return this.http.put('/api/brewd/maintenance',body, options)
  }

  stopMaintenance(): Observable<any> {
    let body = JSON.stringify({'mode': 'stop'});
    let headers = new HttpHeaders({'Content-type': 'application/json'});
    let options = {'headers': headers};

    return this.http.put('/api/brewd/maintenance',body, options)
  }

  setMaintenance(mtpump:boolean, heat:boolean, bkpump:boolean, temp:number): Observable<any> {
    let body = JSON.stringify({'mtpump': mtpump,
			       'bkpump': bkpump,
			       'heat': heat,
			       'temp': temp});
    let headers = new HttpHeaders({'Content-type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/brewd/maintenance',body, options)
  }

  getConfig(): Observable<apiConfig> {
    return this.http.get('/api/brewd/config')
      .pipe(
	map(res => (<apiConfigResponse>res).data)
      );
  }

  setConfig(cfg: apiConfig): Observable<{}> {
    let body = JSON.stringify(cfg);
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    //console.log('calling /api/brewd/state', body, options);

    return this.http.post('/api/brewd/config', body, options);
  }

  announceUpdate() {
    this.updateAnnounce$.next(true);
  }

  getPrograms(): Observable<apiProgram[]> {
    return this.http.get('/api/programs')
      .pipe(
	map(res => (<apiProgramsResponse>res).data)
      );
  }

  getProgram(progid: number): Observable<apiProgram> {
    return this.http.get(`/api/programs/${progid}`)
      .pipe(
	map(res => (<apiProgramResponse>res).data)
      );
  }

  delProgram(progid: number): Observable<apiProgramDeleteResponse> {
    return this.http.delete(`/api/programs/${progid}`)
      .pipe(
	map(res => <apiProgramDeleteResponse>res)
      );
  }

  addProgram(data: apiProgram): Observable<apiAddProgramData> {
    let body = JSON.stringify(data);
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/programs', body, options).
      pipe(
	map(res => (<apiAddProgramResponse>res).data)
      );
  }

  saveProgram(data: apiProgram): Observable<apiSaveProgramData> {
    let body = JSON.stringify(data);
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post(`/api/programs/${data.id}`, body, options).
      pipe(
	map(res => (<apiSaveProgramResponse>res).data)
      );
  }

  hasMalt(): Observable<{}> {
    let body = JSON.stringify({'command': 'hasMalt'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    //console.log('calling /api/brewd/state', body, options);

    return this.http.post('/api/brewd/state', body, options);
  }

  spargeDone(): Observable<{}> {
    let body = JSON.stringify({'command': 'spargeDone'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    //console.log('calling /api/brewd/state', body, options);

    return this.http.post('/api/brewd/state', body, options);
  }

  coolingDone(): Observable<{}> {
    let body = JSON.stringify({'command': 'coolingDone'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    //console.log('calling /api/brewd/state', body, options);

    return this.http.post('/api/brewd/state', body, options);
  }

  transferDone(): Observable<{}> {
    let body = JSON.stringify({'command': 'transferDone'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    //console.log('calling /api/brewd/state', body, options);

    return this.http.post('/api/brewd/state', body, options);
  }

  getVolume(): Observable<apiBrewStateVolumeData> {
    return this.http.get('/api/brewd/state/volume').
      pipe(
	map(res => (<apiBrewStateVolume>res).data)
      );
  }

  setVolume(volume: number): Observable<{}> {
    let body = JSON.stringify({'volume': volume});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    console.log("Setting volume to ", volume, body, headers, options);

    return this.http.post('/api/brewd/state/volume', body, options);
  }

  startBoil(): Observable<{}> {
    let body = JSON.stringify({'command': 'startBoil'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    //console.log('calling /api/brewd/state', body, options);

    return this.http.post('/api/brewd/state', body, options);
  }

  abortBrew(): Observable<{}> {
    let body = JSON.stringify({'command': 'reset'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    //console.log('calling /api/brewd/state', body, options);

    return this.http.post('/api/brewd/state', body, options);
  }

  override(blockheat: boolean, forcemtpump: boolean, bkpump: boolean): Observable<{}> {
    let body = JSON.stringify({'blockheat': blockheat,
			       'forcemtpump': forcemtpump,
			       'bkpump': bkpump});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/brewd/override', body, options);
  }

  setCoolTemp(cooltemp: number): Observable<{}> {
    let body = JSON.stringify({'cooltemp': cooltemp});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    //console.log("Setting volume to ", volume, body, headers, options);

    return this.http.post('/api/brewd/state/cooltemp', body, options);
  }

  loadProgram(data: apiBrewLoadProgramRequest): Observable<{}> {
    let body = JSON.stringify(data);
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/brewd/program', body, options);
  }

}

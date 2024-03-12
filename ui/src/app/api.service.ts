import { Injectable } from '@angular/core';

import { HttpClient, HttpParams, HttpHeaders,
	 HttpErrorResponse } from '@angular/common/http';

import { timer, Observable, BehaviorSubject, throwError,
	 interval } from 'rxjs';
import { map, catchError } from 'rxjs/operators';

import { apiResponse,
	 apiStateData, apiConfig, apiProgram,
	 apiAddProgram, apiSaveProgramData,
	 apiBrewStateVolume, apiBrewStateVolumeData,
	 apiBrewTempHistory, apiBrewLoadProgramRequest,
	 apiFermd, apiTilthydrometer,
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
  public temphistory$ = new BehaviorSubject<apiBrewTempHistory|null>(null);
  private temphistory_last:number = 0;
  private temphistory_data:apiBrewTempHistory|null = null;
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
  public fermds$ = new BehaviorSubject<apiFermd[]|null>(null);
  private timer_fermds;
  private timer_fermds_sub;

  constructor(private http: HttpClient) {
    //console.log('ApiService ctor');
    this.timer_state = timer(1000, 1000);
    this.timer_state_sub = this.timer_state.subscribe(
      (t:any) => {this.updateState(t)}
    );
    this.timer_temphistory = timer(1000, 1000);
    this.timer_temphistory_sub = this.timer_temphistory.subscribe(
      (t:any) => {
	if ( t%5 == 0 ) this.updateTempHistory(t)
      }
    );
    this.timer_fermds = timer(100, 1000);
    this.timer_fermds_sub = this.timer_state.subscribe(
      (t:any) => {
	if ( t%10 == 0 ) this.updateFermds(t);
      }
    );
  }

  private handleErrors(error: HttpErrorResponse) {
    //console.log("Error code: ", error.status);
    return throwError(() => new Error('lofasz'));
  }

  updateState(t:any) {
    this.http.get('/api/brewd/state')
      .pipe(
	catchError(this.handleErrors),
	map(res => <apiResponse>res)
      )
      .subscribe((res:apiResponse) => {
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
	map(res => <apiBrewTempHistory>((<apiResponse>res).data))
      )
      .subscribe(
	(data:apiBrewTempHistory) => {
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

  getAllTempHistory(): apiBrewTempHistory {
    return this.temphistory_data as apiBrewTempHistory;
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
	catchError(this.handleErrors),
	map(res => <apiConfig>((<apiResponse>res).data))
      );
  }

  setConfig(cfg: apiConfig): Observable<{}> {
    let body = JSON.stringify(cfg);
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/brewd/config', body, options);
  }

  announceUpdate() {
    this.updateAnnounce$.next(true);
  }

  getPrograms(): Observable<apiProgram[]> {
    return this.http.get('/api/programs')
      .pipe(
	map(res => <apiProgram[]>((<apiResponse>res).data))
      );
  }

  getProgram(progid: number): Observable<apiProgram> {
    return this.http.get(`/api/programs/${progid}`)
      .pipe(
	map(res => <apiProgram>((<apiResponse>res).data))
      );
  }

  delProgram(progid: number): Observable<apiResponse> {
    return this.http.delete(`/api/programs/${progid}`)
      .pipe(
	map(res => <apiResponse>res)
      );
  }

  addProgram(data: apiProgram): Observable<apiAddProgram> {
    let body = JSON.stringify(data);
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/programs', body, options).
      pipe(
	map(res => <apiAddProgram>((<apiResponse>res).data))
      );
  }

  saveProgram(data: apiProgram): Observable<apiSaveProgramData> {
    let body = JSON.stringify(data);
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post(`/api/programs/${data.id}`, body, options).
      pipe(
	map(res => <apiSaveProgramData>((<apiResponse>res).data))
      );
  }

  hasMalt(): Observable<{}> {
    let body = JSON.stringify({'command': 'hasMalt'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/brewd/state', body, options);
  }

  spargeDone(): Observable<{}> {
    let body = JSON.stringify({'command': 'spargeDone'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/brewd/state', body, options);
  }

  coolingDone(): Observable<{}> {
    let body = JSON.stringify({'command': 'coolingDone'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/brewd/state', body, options);
  }

  transferDone(): Observable<{}> {
    let body = JSON.stringify({'command': 'transferDone'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/brewd/state', body, options);
  }

  getVolume(): Observable<apiBrewStateVolumeData> {
    return this.http.get('/api/brewd/state/volume').
      pipe(
	catchError(this.handleErrors),
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

    return this.http.post('/api/brewd/state', body, options);
  }

  abortBrew(): Observable<{}> {
    let body = JSON.stringify({'command': 'reset'});
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

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

    return this.http.post('/api/brewd/state/cooltemp', body, options);
  }

  loadProgram(data: apiBrewLoadProgramRequest): Observable<{}> {
    let body = JSON.stringify(data);
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};

    return this.http.post('/api/brewd/program', body, options);
  }

  updateFermds(t: any) {
    this.http.get('/api/fermds').
      pipe(
	catchError(this.handleErrors),
	map(res => <apiFermd[]>((<apiResponse>res).data))
      ).subscribe(
	(data:apiFermd[]) => {
	  this.fermds$.next(data);
	},
	(err:any) => {
	  this.fermds$.next(null);
	}
      );
  }

  addFermd(data: apiFermd): Observable<apiFermd> {
    let body = JSON.stringify(data);
    let headers = new HttpHeaders({'Content-Type': 'application/json'});
    let options = {'headers': headers};
    return this.http.post('/api/fermds', body, options)
      .pipe(
	map(res => <apiFermd>((<apiResponse>res).data))
      );
  }

  getTilthydrometers(fermdid: number): Observable<apiTilthydrometer[]> {
    return this.http.get(`/api/fermds/${fermdid}/tilthydrometers`)
      .pipe(
	catchError(this.handleErrors),
	map(res => <apiTilthydrometer[]>((<apiResponse>res).data))
      )
  }

}

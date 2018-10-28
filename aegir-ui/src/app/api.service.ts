import { Injectable } from '@angular/core';
import { HttpClient, HttpParams, HttpHeaders } from '@angular/common/http';
import { timer, Observable, Subject } from 'rxjs';
import {map } from 'rxjs/operators';

import { Program } from './programs/program';


export class ApiResponse {
    constructor(
	public status: string,
	public uri: string,
	public errors: string[]) {
    }
}

@Injectable()
export class ApiService {

    private static instance: ApiService = null;

    // Observable resources
    private updateSource = new Subject<boolean>();
    // observable streams
    updateAnnounce$ = this.updateSource.asObservable();
    // state timer
    private timer_state;
    private timer_state_sub;
    private timer_temphistory;
    private timer_temphistory_sub;
    // state observable
    private state = new Subject();
    private state_data = 'Empty';
    private temphistory = new Subject();
    private temphistory_data = null;

    constructor(private http: HttpClient) {
	if ( ApiService.instance != null ) return ApiService.instance;
	ApiService.instance = this;
	this.timer_state = timer(1000,1000);
	this.timer_state_sub = this.timer_state.subscribe(t => {this.updateState(t)});

	this.timer_temphistory = timer(1000,5000);
	this.timer_temphistory_sub = this.timer_temphistory.subscribe(t => {this.updateTempHistory(t)});
    }

    getState(): Observable<{}> {
	return this.state.asObservable();
    }

    getTempHistory(): Observable<{}> {
	return this.temphistory.asObservable();
    }

    updateState(t) {
	this.http.get(`/api/brewd/state`)
	    .subscribe(res => {
		//let rjson = res.json()['data'];
		let rjson = res['data'];
		//console.log('status', rjson);
		let state = rjson['state'];
		this.state_data = state;
		let newstates = new Set(['Empty', 'Loaded', 'PreWait', 'PreHeat', 'NeedMalt']);
		if ( newstates.has(state) ) {
		    this.temphistory_data = null;
		}
		this.state.next(rjson);
	    });
    }

    updateTempHistory(t) {
	let newstates = new Set(['Maintenance', 'Empty', 'Loaded', 'PreWait', 'PreHeat', 'NeedMalt',
				 'PreBoil', 'Hopping', 'Cooling', 'Finished']);
	if ( newstates.has(this.state_data) ) return;
	console.log('state data', this.state_data);

	/*
	let params: URLSearchParams = new URLSearchParams();
	let frm = 0;
	if ( this.temphistory_data != null ) {
	    frm = this.temphistory_data['timestamps'][this.temphistory_data['timestamps'].length-1]+1;
	}
	params.set('from', frm.toString());
	*/
	let params = new HttpParams();
	let frm = 0;
	if ( this.temphistory_data != null ) {
	    frm = this.temphistory_data['timestamps'][this.temphistory_data['timestamps'].length-1]+1;
	}
	params.set('from', frm.toString());

	this.http.get(`/api/brewd/state/temphistory`, {params: params})
	    .subscribe(res => {
		//let rjson = res.toJSON()['data'];
		let rjson = res['data']
		//console.log('got temphistory', rjson, this.temphistory_data);
		if ( this.temphistory_data == null ) {
		    this.temphistory_data = rjson;
		} else {
		    // merge it to the temphistory_data
		    // timestamps
		    for ( let i of rjson['timestamps'] ) {
			this.temphistory_data['timestamps'].push(i);
		    }
		    // sensors
		    for ( let sensor in rjson['readings'] ) {
			for ( let value of rjson['readings'][sensor] ) {
			    this.temphistory_data['readings'][sensor].push(value);
			}
		    }
		}
		this.temphistory.next(this.temphistory_data);
	    });
    }

    announceUpdate() {
	//console.log('API announcing update');
	this.updateSource.next(true);
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

    getVolume(): Observable<{}> {
	return this.http.get('/api/brewd/state/volume');
    }

    setVolume(volume): Observable<{}> {
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

    getPrograms(): Observable<Program[]> {
	//return this.http.get<Program[]>('/api/programs');
	return this.http.get('/api/programs').pipe(
	    map(resp => {
		return resp['data'];
	    }));
    }

    addProgram(data: Object): Observable<ApiResponse> {
	data['nomash'] = !data['hasmash'];
	data['noboil'] = !data['hasboil'];

	let body = JSON.stringify(data);
	let headers = new HttpHeaders({'Content-Type': 'application/json'});
	let options = {'headers': headers};

	return this.http.post<ApiResponse>('/api/programs', body, options);
    }

    saveProgram(data: Object): Observable<ApiResponse> {
	data['nomash'] = !data['hasmash'];
	data['noboil'] = !data['hasboil'];

	let body = JSON.stringify(data);
	let headers = new HttpHeaders({'Content-Type': 'application/json'});
	let options = {'headers': headers};

	return this.http.post<ApiResponse>(`/api/programs/${data['id']}`, body, options);
    }

    delProgram(progid: number): Observable<ApiResponse> {
	return this.http.delete<ApiResponse>(`/api/programs/${progid}`);
    }

    getProgram(progid: number): Observable<Program> {
	return this.http.get<Program>(`/api/programs/${progid}`);
    }

    loadProgram(data: Object): Observable<ApiResponse> {
	let body = JSON.stringify(data);
	let headers = new HttpHeaders({'Content-Type': 'application/json'});
	let options = {'headers': headers};

	return this.http.post<ApiResponse>('/api/brewd/program', body, options);
    }

    private handleError(error: Response | any ) {
	let errMsg: string;
	/*
	if (error instanceof Response) {
	    const body = error.json() || '';
	    const err = body.error || JSON.stringify(body);
	    errMsg = `${error.status} - ${error.statusText || ''} ${err}`;
	} else {
	    errMsg = error.message ? error.message : error.toString();
	}
	console.log(errMsg);
	return Observable.throw(errMsg);
	*/
    }

    startMaintenance(): Observable<{}> {
	let body = JSON.stringify({'mode': 'start'});
	let headers = new HttpHeaders({'Content-Type': 'application/json'});
	let options = {'headers': headers};

	return this.http.put('/api/brewd/maintenance', body, options);
    }

    stopMaintenance(): Observable<{}> {
	let body = JSON.stringify({'mode': 'stop'});
	let headers = new HttpHeaders({'Content-Type': 'application/json'});
	let options = {'headers': headers};

	return this.http.put('/api/brewd/maintenance', body, options);
    }

    setMaintenance(pump, heat, temp): Observable<{}> {
	let body = JSON.stringify({'pump': pump,
				   'heat': heat,
				   'temp': temp});
	let headers = new HttpHeaders({'Content-Type': 'application/json'});
	let options = {'headers': headers};

	//console.log("Setting maint to ", pump, heat, temp, body, headers, options);

	return this.http.post('/api/brewd/maintenance', body, options);
    }

    override(blockheat, forcepump): Observable<{}> {
	let body = JSON.stringify({'blockheat': blockheat,
				   'forcepump': forcepump});
	let headers = new HttpHeaders({'Content-Type': 'application/json'});
	let options = {'headers': headers};

	return this.http.post('/api/brewd/override', body, options);
    }
}

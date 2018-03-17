import { Injectable } from '@angular/core';
import { Headers, Http, Response, RequestOptions, URLSearchParams } from '@angular/http';
import { Observable } from 'rxjs/Rx';

import { Program } from './programs/program';

import { Subject } from 'rxjs/Subject';
import 'rxjs/add/operator/map';
import 'rxjs/add/operator/catch';

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
    private timer_temphistory;
    // state observable
    private state = new Subject();
    private state_data = 'Empty';
    private temphistory = new Subject();
    private temphistory_data = null;

    constructor(private http: Http) {
	if ( ApiService.instance != null ) return ApiService.instance;
	ApiService.instance = this;
	this.timer_state = Observable.timer(1000,1000);
	this.timer_state.subscribe(t => {this.updateState(t)});

	this.timer_temphistory = Observable.timer(1000,5000);
	this.timer_temphistory.subscribe(t => {this.updateTempHistory(t)});
    }

    getState(): Observable<{}> {
	return this.state.asObservable();
    }

    getTempHistory(): Observable<{}> {
	return this.temphistory.asObservable();
    }

    updateState(t) {
	let params: URLSearchParams = new URLSearchParams();

	this.http.get(`/api/brewd/state`, {search: params})
	    .subscribe(res => {
		let rjson = res.json()['data'];
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

	let params: URLSearchParams = new URLSearchParams();
	let frm = 0;
	if ( this.temphistory_data != null ) {
	    frm = this.temphistory_data['timestamps'][this.temphistory_data['timestamps'].length-1]+1;
	}
	params.set('from', frm.toString());

	this.http.get(`/api/brewd/state/temphistory`, {search: params})
	    .subscribe(res => {
		let rjson = res.json()['data'];
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
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	console.log('calling /api/brewd/state', body, options);

	return this.http.post('/api/brewd/state', body, options)
	    .map((res:Response) => {
		console.log('Catching result, ', res.status);
		return res.json();
	    });
    }

    spargeDone(): Observable<{}> {
	let body = JSON.stringify({'command': 'spargeDone'});
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	console.log('calling /api/brewd/state', body, options);

	return this.http.post('/api/brewd/state', body, options)
	    .map((res:Response) => {
		console.log('Catching result, ', res.status);
		return res.json();
	    });
    }

    getVolume(): Observable<{}> {
	return this.http.get('/api/brewd/state/volume')
	    .map(this.extractData)
	    .catch(this.handleError);
    }

    setVolume(volume): Observable<{}> {
	let body = JSON.stringify({'volume': volume});
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	console.log("Setting volume to ", volume, body, headers, options);

	return this.http.post('/api/brewd/state/volume', body, options)
	    .map((res:Response) => {
		console.log('Catching result, ', res.status);
		return res.json();
	    })
	    .catch(this.handleError);
    }

    startBoil(): Observable<{}> {
	let body = JSON.stringify({'command': 'startBoil'});
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	console.log('calling /api/brewd/state', body, options);

	return this.http.post('/api/brewd/state', body, options)
	    .map((res:Response) => {
		console.log('Catching result, ', res.status);
		return res.json();
	    });
    }

    abortBrew(): Observable<{}> {
	let body = JSON.stringify({'command': 'reset'});
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	console.log('calling /api/brewd/state', body, options);

	return this.http.post('/api/brewd/state', body, options)
	    .map((res:Response) => {
		console.log('Catching result, ', res.status);
		return res.json();
	    });
    }

    getPrograms(): Observable<Program[]> {
	return this.http.get('/api/programs')
	    .map(this.extractData)
	    .catch(this.handleError);
    }

    addProgram(data: Object): Observable<ApiResponse> {
	data['nomash'] = !data['hasmash'];
	data['noboil'] = !data['hasboil'];

	let body = JSON.stringify(data);
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	return this.http.post('/api/programs', body, options)
	    .map((res:Response) => {
		//console.log('Catching result, ', res.status);
		return res.json();
	    });
	    //.catch((error:any) => Observable.throw(error.json().error || 'Servererror'));
    }

    saveProgram(data: Object): Observable<ApiResponse> {
	data['nomash'] = !data['hasmash'];
	data['noboil'] = !data['hasboil'];

	let body = JSON.stringify(data);
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	return this.http.post(`/api/programs/${data['id']}`, body, options)
	    .map((res:Response) => {
		//console.log('Catching result, ', res.status);
		return res.json();
	    });
	    //.catch((error:any) => Observable.throw(error.json().error || 'Servererror'));
    }

    delProgram(progid: number): Observable<ApiResponse> {
	return this.http.delete(`/api/programs/${progid}`)
	    .map((res:Response) => {
		//console.log('Delete result, ', res.status);
		return res.json();
	    });
	    //.catch((error:any) => Observable.throw(error.json().error || 'Servererror'));
    }

    getProgram(progid: number): Observable<Program> {
	return this.http.get(`/api/programs/${progid}`)
	    .map((res:Response) => res.json());
	    //.catch((error:any) => Observable.throw(error.json().error || 'Servererror'));
    }

    loadProgram(data: Object): Observable<ApiResponse> {
	let body = JSON.stringify(data);
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	return this.http.post('/api/brewd/program', body, options)
	    .map((res:Response) => {
		//console.log('Catching result, ', res.status);
		return res.json();
	    });
	    //.catch((error:any) => Observable.throw(error.json().error || 'Servererror'));
    }

    private extractData(res: Response) {
	let body = res.json()
	return body.data || {};
    }

    private handleError(error: Response | any ) {
	let errMsg: string;
	if (error instanceof Response) {
	    const body = error.json() || '';
	    const err = body.error || JSON.stringify(body);
	    errMsg = `${error.status} - ${error.statusText || ''} ${err}`;
	} else {
	    errMsg = error.message ? error.message : error.toString();
	}
	console.log(errMsg);
	return Observable.throw(errMsg);
    }

    startMaintenance(): Observable<{}> {
	let body = JSON.stringify({'mode': 'start'});
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	return this.http.put('/api/brewd/maintenance', body, options)
	    .map((res:Response) => {
		//console.log('Catching result, ', res.status);
		return res.json();
	    });
    }

    stopMaintenance(): Observable<{}> {
	let body = JSON.stringify({'mode': 'stop'});
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	return this.http.put('/api/brewd/maintenance', body, options)
	    .map((res:Response) => {
		//console.log('Catching result, ', res.status);
		return res.json();
	    });
    }

    setMaintenance(pump, heat, temp): Observable<{}> {
	let body = JSON.stringify({'pump': pump,
				   'heat': heat,
				   'temp': temp});
	let headers = new Headers({'Content-Type': 'application/json'});
	let options = new RequestOptions({headers: headers});

	//console.log("Setting maint to ", pump, heat, temp, body, headers, options);

	return this.http.post('/api/brewd/maintenance', body, options)
	    .map((res:Response) => {
		console.log('Catching result, ', res.status);
		return res.json();
	    })
	    .catch(this.handleError);
    }
}

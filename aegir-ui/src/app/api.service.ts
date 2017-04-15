import { Injectable } from '@angular/core';
import { Headers, Http, Response, RequestOptions } from '@angular/http';

import { Program } from './programs/program';

import { Observable } from 'rxjs/Observable';
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

    // Observable resources
    private updateSource = new Subject<boolean>();
    // observable streams
    updateAnnounce$ = this.updateSource.asObservable();

    constructor(private http: Http) { }

    announceUpdate() {
	//console.log('API announcing update');
	this.updateSource.next(true);
    }

    getPrograms(): Observable<Program[]> {
	return this.http.get('/api/programs')
	    .map(this.extractData)
	    .catch(this.handleError);
    }

    addProgram(data: Object): Observable<ApiResponse> {
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
}

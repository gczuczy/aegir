import { Injectable } from '@angular/core';
import { Headers, Http, Response } from '@angular/http';

import { Program } from './program';

import { Observable } from 'rxjs/Observable';
import 'rxjs/add/operator/map';
import 'rxjs/add/operator/catch';

@Injectable()
export class ApiService {

    constructor(private http: Http) { }

    getPrograms(): Observable<Program[]> {
	return this.http.get('/api/programs')
	    .map(this.extractData)
	    .catch(this.handleError);
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

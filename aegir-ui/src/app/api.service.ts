import { Injectable } from '@angular/core';

import { HttpClient, HttpParams, HttpHeaders } from '@angular/common/http';

import { timer, Observable, BehaviorSubject } from 'rxjs';
import { map } from 'rxjs/operators';

import { apiStateResponse, apiStateData, apiConfigResponse, apiConfig } from './api.types';

@Injectable({
  providedIn: 'root'
})
export class ApiService {

  // observable state
  private timer_state;
  private timer_state_sub;
  private state_data = 'Empty';
  private temphistory_data = null;
  private state = new BehaviorSubject(<apiStateData>{
    levelerror: false,
    state: 'Empty',
    targettemp: 0,
    currtemp: {
      BK: 0,
      HLT: 0,
      MashTun: 0,
      RIMS: 0
    }
  });

  constructor(private http: HttpClient) {
    //console.log('ApiService ctor');
    this.timer_state = timer(1000, 1000);
    this.timer_state_sub = this.timer_state.subscribe((t:any) => {this.updateState(t)});
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
	  this.temphistory_data = null;
	}
	this.state.next(res.data);
      }, (err:any) => {
	console.log('updateState/err', err);
      });
  }

  getState(): BehaviorSubject<apiStateData> {
    return this.state;
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

}

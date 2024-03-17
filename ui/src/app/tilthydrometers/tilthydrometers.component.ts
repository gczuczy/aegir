import { Component, OnInit, OnDestroy } from '@angular/core';
import { ActivatedRoute, Params, Router } from '@angular/router';
import { timer } from 'rxjs';
import { switchMap } from 'rxjs/operators';

import { ApiService } from '../api.service';
import { apiTilthydrometer } from '../api.types';

@Component({
  selector: 'app-tilthydrometers',
  templateUrl: './tilthydrometers.component.html',
  styleUrls: ['./tilthydrometers.component.css']
})
export class TilthydrometersComponent implements OnInit, OnDestroy {
  public isopen_assigned=true;
  public isopen_enabled=true;

  private fermdid: number|null = null;
  private timer : any;
  private timer_sub : any;

  private tilts: apiTilthydrometer[]|null = null;

  constructor(private api: ApiService,
    private route: ActivatedRoute,
    private router: Router) {
    this.timer = timer(1000, 1000);
  }

  ngOnInit() {
    this.route.parent!.params
      .subscribe(
	(params:Params) => {
	  this.fermdid = params['fermdid'];
	}
      );
    this.timer_sub = this.timer.subscribe(
      (t:any) => {
	if ( t%30 == -1 ) this.updateTilts();
      }
    );
    this.updateTilts();
  }

  ngOnDestroy() {
    this.timer_sub.unsubscribe();
  }

  updateTilts() {
    if ( this.fermdid == null ) return;
    this.api.getTilthydrometers(this.fermdid!).subscribe(
      (data:apiTilthydrometer[]) => {
	let ts: apiTilthydrometer[] = [];
	for ( let t of data ) {
	  if ( t.fermenter === "" ) t.fermenter = undefined;
	  ts.push(t);
	}
	this.tilts = ts;
      }
    );
  }

  assignedTilts(): apiTilthydrometer[] {
    if ( this.tilts == null ) return [];

    let ts: apiTilthydrometer[] = [];

    for (let t of this.tilts) {
      if ( t.fermenter != undefined ) {
	ts.push(t);
      }
    }
    return ts;
  }

  enabledTilts(): apiTilthydrometer[] {
    if ( this.tilts == null ) return [];

    let ts: apiTilthydrometer[] = [];

    for (let t of this.tilts) {
      if ( t.enabled ) {
	ts.push(t);
      }
    }
    return ts;
  }

  disabledTilts(): apiTilthydrometer[] {
    if ( this.tilts == null ) return [];

    let ts: apiTilthydrometer[] = [];

    for (let t of this.tilts) {
      if ( !t.enabled ) {
	ts.push(t);
      }
    }
    return ts;
  }

  enableTilt(tiltid: number) {
    let data = new Map<string, string|number|boolean>();
    data.set('enabled', true);
    this.api.updateTilthydrometer(this.fermdid!,
				  tiltid, data)
      .subscribe(
	(data:apiTilthydrometer) => {
	  this.updateTilts();
	}
      );
  }

  disableTilt(tiltid: number) {
    let data = new Map<string, string|number|boolean>();
    data.set('enabled', false);
    this.api.updateTilthydrometer(this.fermdid!,
				  tiltid, data)
      .subscribe(
	(data:apiTilthydrometer) => {
	  this.updateTilts();
	}
      );
  }
}

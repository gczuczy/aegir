import { Component, OnInit } from '@angular/core';
import { ActivatedRoute, Params, Router } from '@angular/router';
import { switchMap } from 'rxjs/operators';

import { ApiService } from './api.service';
import { apiFermdData } from './api.types';


@Component({
  selector: 'app-fermd',
  templateUrl: './fermd.component.html',
  styleUrls: ['./fermd.component.css']
})
export class FermdComponent implements OnInit {

  private fermdid: number|null = null;
  public fermd: apiFermdData|null = null;
  private fermds: apiFermdData[]|null = null;

  constructor(private api: ApiService,
    private route: ActivatedRoute,
    private router: Router) {
    }

  ngOnInit() {
    this.api.fermds$.subscribe(
      (data:apiFermdData[]|null) => {
	this.fermds = data;
	if ( this.fermdid != null && this.fermd == null && this.fermds != null)
	  this.setFermd();
      }
    );
    this.route.params
      .subscribe(
	(params:Params) => {
	  this.fermdid = params['fermdid'];
	  if ( this.fermds != null ) this.setFermd();
	}
      );
  }

  setFermd() {
    console.log('Loading fermd', this.fermdid, this.fermds)
    if ( this.fermdid == null ) return;
    if ( this.fermds == null ) return;
    for (var fermd of this.fermds!) {
      if ( fermd.id! == this.fermdid ) {
	this.fermd = fermd;
	break;
      }
    }
  }

}

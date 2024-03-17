import { Component, OnInit } from '@angular/core';
import { ActivatedRoute, Params, Router } from '@angular/router';
import { switchMap } from 'rxjs/operators';

import { ApiService } from './api.service';
import { apiFermd } from './api.types';


@Component({
  selector: 'app-fermd',
  templateUrl: './fermd.component.html',
  styleUrls: ['./fermd.component.css']
})
export class FermdComponent implements OnInit {

  private fermdid: number|null = null;
  public fermd: apiFermd|null = null;
  public fermds: apiFermd[]|null = null;

  constructor(private api: ApiService,
    private route: ActivatedRoute,
    private router: Router) {
    }

  ngOnInit() {
    this.api.fermds$.subscribe(
      (data:apiFermd[]|null) => {
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

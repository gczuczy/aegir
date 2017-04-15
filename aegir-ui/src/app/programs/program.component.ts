import { Component, OnInit } from '@angular/core';
import { ActivatedRoute, Params } from '@angular/router';
import { Router } from '@angular/router';
import 'rxjs/add/operator/switchMap';

import { Program } from './program';
import { ApiService } from '../api.service';

@Component({
  templateUrl: './program.component.html',
  styleUrls: ['./program.component.css']
})
export class ProgramComponent implements OnInit {
    public program: Program;

    constructor(private api: ApiService,
		private route: ActivatedRoute,
		private router: Router) {
    }

    ngOnInit() {
	this.route.params
	    .switchMap((params: Params) => this.api.getProgram(params['id']))
	    .subscribe(prog => {
		this.program = prog['data'];
		//console.log('Loaded prog: ', this.program);
	    });
    }

    onEdit() {
	this.router.navigate(['programs', this.program.id, 'edit']);
    }

    onDelete() {
	if ( confirm("Are you sure to delete this program?") ) {
	    this.api.delProgram(this.program.id).subscribe(
		resp => {
		    this.api.announceUpdate();
		    this.router.navigate(['programs']);
		    //console.log('Program Added with response: ', resp);
		},
		err => {
		    console.log('Error obj: ', err);
		}
	    );
	    console.log("Delete ", this.program);
	}
    }
}

import { Component, OnInit } from '@angular/core';
import { Router } from '@angular/router';

import { Program } from './program';
import { ApiService } from '../api.service';

@Component({
    selector: 'app-programs',
    templateUrl: './programs.component.html',
    styleUrls: ['./programs.component.css'],
    providers: [ ApiService ]
})
export class ProgramsComponent implements OnInit {
    errorMessage: string;
    programs: Program[];

    constructor(private apiService: ApiService,
		private router: Router) {
	this.apiService.updateAnnounce$.subscribe(
	    u => {
		this.updateList();
	    }
	);
    }

    ngOnInit() {
	this.getPrograms();
    }

    getPrograms() {
	this.updateList();
    }

    updateList() {
	//console.log('Programs::updateList()');
	this.apiService.getPrograms()
	    .subscribe(
		programs => {
		    this.programs = programs;
		},
		error => this.errorMessage = <any>error);
    }

    loadProgram(prog: Program) {
	//console.log('loadProgram ', prog);
	this.router.navigate(['programs', prog.id, 'view']);
    }

}

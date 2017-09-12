import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';
import { HttpModule } from '@angular/http';
import { RouterModule } from '@angular/router';
import { APP_BASE_HREF } from '@angular/common';

import { BrewModule } from './brew/brew.module';
import { ProgramsModule } from './programs/programs.module';
import { SettingsModule } from './settings/settings.module';

import { AppComponent } from './app.component';

import { ApiService } from './api.service';

import { APP_ROUTES } from './app.routes';

@NgModule({
    declarations: [
	AppComponent,
    ],
    imports: [
	BrowserModule,
	HttpModule,
	BrewModule,
	SettingsModule,
	ProgramsModule,
	RouterModule.forRoot(APP_ROUTES),
    ],
    providers: [
	ApiService
    ],
    bootstrap: [AppComponent]
})
export class AppModule { }
